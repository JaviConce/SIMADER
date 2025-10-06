#include "mqtt_Manager.hpp"
#include "web_html.h"
#include <Arduino.h>

MQTTBroker broker;
WiFiClient wifiClient;
PubSubClient client(wifiClient);
WebServer server(80);


void taskMQTTBrokerClientManager(void *parameter) {
    while(true) {
        broker.loop();
        vTaskDelay(10 / portTICK_PERIOD_MS); // Delay to prevent watchdog timer reset
    }
    vTaskDelete(NULL);
}

int initMQTT_Server() {
    broker.begin(mqttPort);
    Serial.println("[MQTT_MANAGER][INFO] MQTT Broker initialized.");
    return 1;
}

void callback(char* topic, byte* payload, unsigned int length){
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

void connectBroker(std::string mqttBroker) {
    client.setServer(mqttBroker.c_str(), mqttPort);
    client.setCallback(callback);

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

void handleRoot() {
    server.send(200, "text/html", index_html);
}

void handleSend() {
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

void handleGetClients() {
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

void initWebServer() {
    server.on("/", handleRoot);
    server.on("/send", handleSend);
    server.on("/api/clients", handleGetClients);
    server.begin();
    Serial.printf("[MQTT_MANAGER][WEB] Web Server inicializated in http://%s\n", WiFi.softAPIP().toString().c_str());
}

void handleWebServer() {
    server.handleClient();
}

void handleMQTTClient() {
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