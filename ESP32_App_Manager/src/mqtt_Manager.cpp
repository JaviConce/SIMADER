#include "mqtt_Manager.hpp"
#include "web_html.h"
#include <Arduino.h>

// Instancia global para acceso desde callbacks
MqttManager* mqttManagerInstance = nullptr;

// Tarea FreeRTOS para el broker
void taskMQTTBrokerClientManager(void *parameter) {
    MqttManager* manager = (MqttManager*)parameter;
    while(true) {
        manager->getBroker()->loop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

// Constructor
MqttManager::MqttManager() : client(wifiClient), server(80) {
    mqttManagerInstance = this;
}

// Destructor
MqttManager::~MqttManager() {
    mqttManagerInstance = nullptr;
}

// Inicializar servidor MQTT
int MqttManager::initMQTT_Server() {
    broker.begin(mqttPort);
    Serial.println("[MQTT_MANAGER][INFO] MQTT Broker initialized.");
    return 1;
}

// Callback estático para MQTT
void MqttManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
    std::string msg;
    for(int i=0; i<length; i++) msg += (char)payload[i];

    // Parsear respuesta del Device: <IP_Broker>:<Data>
    if(std::string(topic) == "esp32/responses") {
        int colonIndex = msg.find(':');
        if(colonIndex > 0) {
            std::string sourceIp = msg.substr(0, colonIndex);
            std::string data = msg.substr(colonIndex + 1);
            Serial.printf("[MQTT_MANAGER][RESPONSE] Device response - IP: %s, Data: %s\n",sourceIp.c_str(), data.c_str());
        }
    }
}

// Conectar al broker MQTT
void MqttManager::connectBroker(std::string mqttBroker) {
    client.setBufferSize(512);
    client.setServer(mqttBroker.c_str(), mqttPort);
    client.setCallback(mqttCallback);

    while(!client.connected()){
        Serial.print("[MQTT_MANAGER][INFO] Connecting to the MQTT broker...\n");
        if(client.connect("ESP32ManagerClient")){
            client.subscribe("esp32/responses");
            Serial.println("[MQTT_MANAGER][INFO] Subscribed to esp32/data and esp32/responses");
        } else {
            Serial.printf("[MQTT_MANAGER][ERROR] Failure, rc=%d. Retrying in 2 seconds...\n", client.state());
            delay(2000);
        }
    }
}


// Handler para obtener clientes
void MqttManager::handleGetClients() {
    std::vector<ClientInfo> clients = broker.getConnectedClients();

    std::string json = "{\"clients\":[";
    for(size_t i = 0; i < clients.size(); i++) {
        json += "{";
        json += "\"id\":\"" + std::string(clients[i].clientId.c_str()) + "\",";
        json += "\"ip\":\"" + std::string(clients[i].ipAddress.c_str()) + "\",";
        json += "\"connected\":" + std::string(clients[i].connected ? "true" : "false") + ",";
        json += "\"uptime\":" + std::to_string((millis() - clients[i].connectedTime) / 1000);
        json += "}";
        if(i < clients.size() - 1) json += ",";
    }
    json += "],\"total\":" + std::to_string(clients.size()) + "}";

    server.send(200, "application/json", json.c_str());
}

// Handler para la raíz
void MqttManager::handleRoot() {
    server.send(200, "text/html", index_html);
}

// Handler para enviar comandos
void MqttManager::handleSend() {
    if(!client.connected()) {
        server.send(500, "text/plain", "Error: MQTT client not connected");
        Serial.println("[MQTT_MANAGER][WEB][ERROR] MQTT client not connected");
        return;
    }

    std::string deviceIP = server.arg("ip").c_str();
    std::string command = server.arg("cmd").c_str();

    if(deviceIP.length() == 0 || command.length() == 0) {
        server.send(400, "text/plain", "Error: Missing parameters (ip or cmd)");
        return;
    }

    // Formato: <IP_Receptor>:<SendData|Check>
    std::string message = deviceIP + ":" + command;
    client.publish("esp32/commands", message.c_str());

    std::string response = "Command '" + command + "' send to " + deviceIP;
    server.send(200, "text/plain", response.c_str());
}

// Inicializar servidor web
void MqttManager::initWebServer() {
    server.on("/", [this]() { this->handleRoot(); });
    server.on("/send", [this]() { this->handleSend(); });
    server.on("/api/clients", [this]() { this->handleGetClients(); });
    server.begin();
    Serial.printf("[MQTT_MANAGER][WEB] Web Server inicializated in http://%s\n", WiFi.softAPIP().toString().c_str());
}

// Manejar servidor web
void MqttManager::handleWebServer() {
    server.handleClient();
}

// Manejar cliente MQTT
void MqttManager::handleMQTTClient() {
    if(!client.connected()) {
        Serial.println("[MQTT_MANAGER][WARNING] MQTT client disconnected, reconnecting...");
        if(client.connect("ESP32ManagerClient")) {
            client.subscribe("esp32/data");
            client.subscribe("esp32/responses");
            Serial.println("[MQTT_MANAGER][INFO] MQTT client reconnected");
        }
    }
    client.loop();
}

// Publicar clientes conectados
void MqttManager::publishClients() {
    if(!client.connected()) return;

    std::vector<ClientInfo> clients = broker.getConnectedClients();

    // Filtrar solo sensores (excluir Node-RED, Manager, y clientes sin ID)
    std::string json = "{\"clients\":[";
    int count = 0;
    for(size_t i = 0; i < clients.size(); i++) {
        std::string clientId = std::string(clients[i].clientId.c_str());

        // Filtrar: solo incluir clientes que sean sensores ESP32
        // Excluir: nodered-client, ESP32ManagerClient, y clientes vacíos
        if (clientId.empty() ||
            clientId == "ESP32ManagerClient" ||
            clientId == "nodered-client" ||
            clientId.find("nodered") != std::string::npos) {
            continue;
        }

        if(count > 0) json += ",";

        json += "{";
        json += "\"id\":\"" + clientId + "\",";
        json += "\"ip\":\"" + std::string(clients[i].ipAddress.c_str()) + "\",";
        json += "\"connected\":" + std::string(clients[i].connected ? "true" : "false") + ",";
        json += "\"uptime\":" + std::to_string((millis() - clients[i].connectedTime) / 1000);
        json += "}";

        count++;
    }
    json += "],\"total\":" + std::to_string(count) + "}";
    Serial.printf("[MQTT_MANAGER][INFO] Clients JSON: %s\n", json.c_str());
    client.publish("esp32/clients", json.c_str());
}