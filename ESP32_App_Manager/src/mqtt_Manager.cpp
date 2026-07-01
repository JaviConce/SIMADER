#include "mqtt_Manager.hpp"
#include "config.hpp"
#include <Arduino.h>

// ===================================
// TAREA FREERTOS PARA EL BROKER
// ===================================

void taskMQTTBrokerClientManager(void* parameter) {
    MqttManager* manager = (MqttManager*)parameter;
    while (true) {
        manager->getBroker()->loop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

// ===================================
// CONSTRUCTOR / DESTRUCTOR
// ===================================

MqttManager::MqttManager() : client(wifiClient) {}

MqttManager::~MqttManager() {}

// ===================================
// BROKER MQTT
// ===================================

int MqttManager::initMQTT_Server() {
    broker.begin(MQTT_PORT);
    Serial.println("[MQTT_MANAGER][INFO] MQTT Broker initialized.");
    return 1;
}

// ===================================
// CALLBACK — solo loguea respuestas
// ===================================

void MqttManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
    std::string msg;
    for (int i = 0; i < (int)length; i++) msg += (char)payload[i];

    if (std::string(topic) == "esp32/responses") {
        size_t colonIndex = msg.find(':');
        if (colonIndex != std::string::npos) {
            std::string sourceIp = msg.substr(0, colonIndex);
            std::string data     = msg.substr(colonIndex + 1);
            Serial.printf("[MQTT_MANAGER][RESPONSE] IP: %s, Data: %s\n", sourceIp.c_str(), data.c_str());
        }
    }
}

// ===================================
// CONEXIÓN AL BROKER
// ===================================

void MqttManager::connectBroker(std::string mqttBroker) {
    client.setBufferSize(MQTT_BUFFER_SIZE);
    client.setServer(mqttBroker.c_str(), MQTT_PORT);
    client.setCallback(mqttCallback);

    while (!client.connected()) {
        Serial.println("[MQTT_MANAGER][INFO] Connecting to the MQTT broker...");
        if (client.connect(MQTT_CLIENT_ID)) {
            client.subscribe("esp32/responses", 1);
            Serial.println("[MQTT_MANAGER][INFO] Connected and subscribed to esp32/responses");
        } else {
            Serial.printf("[MQTT_MANAGER][ERROR] Failure rc=%d. Retrying in 2s...\n", client.state());
            delay(2000);
        }
    }
}

// ===================================
// CLIENTE MQTT (loop + reconexión)
// ===================================

void MqttManager::handleMQTTClient() {
    if (!client.connected()) {
        Serial.println("[MQTT_MANAGER][WARNING] MQTT client disconnected, reconnecting...");
        if (client.connect(MQTT_CLIENT_ID)) {
            client.subscribe("esp32/responses", 1);
            Serial.println("[MQTT_MANAGER][INFO] MQTT client reconnected");
        }
    }
    client.loop();
}

// ===================================
// PUBLICAR LISTA DE CLIENTES
// ===================================

void MqttManager::publishClients() {
    if (!client.connected()) return;

    std::vector<ClientInfo> clients = broker.getConnectedClients();

    std::string json = "{\"clients\":[";
    int count = 0;
    for (size_t i = 0; i < clients.size(); i++) {
        std::string clientId = std::string(clients[i].clientId.c_str());

        if (clientId.empty() ||
            clientId == MQTT_CLIENT_ID ||
            clientId.find("nodered") != std::string::npos) {
            continue;
        }

        if (count > 0) json += ",";
        json += "{";
        json += "\"id\":\""      + clientId + "\",";
        json += "\"ip\":\""      + std::string(clients[i].ipAddress.c_str()) + "\",";
        json += "\"connected\":" + std::string(clients[i].connected ? "true" : "false") + ",";
        json += "\"uptime\":"    + std::to_string((millis() - clients[i].connectedTime) / 1000);
        json += "}";
        count++;
    }
    json += "],\"total\":" + std::to_string(count) + "}";

    Serial.printf("[MQTT_MANAGER][INFO] Publishing clients: %s\n", json.c_str());
    client.publish("esp32/clients", json.c_str());
}
