#include "web_server_Manager.hpp"
#include "mqttBroker.hpp"
#include "web_html.h"
#include <Arduino.h>
#include <WiFi.h>

WebServerManager::WebServerManager(PubSubClient& mqttClient, MQTTBroker& broker)
    : _server(80), _mqttClient(mqttClient), _broker(broker) {}

void WebServerManager::init() {
    _server.on("/",           [this]() { handleRoot(); });
    _server.on("/send",       [this]() { handleSend(); });
    _server.on("/api/clients",[this]() { handleGetClients(); });
    _server.begin();
    Serial.printf("[WEB_SERVER][INFO] Web Server started at http://%s\n", WiFi.softAPIP().toString().c_str());
}

void WebServerManager::handle() {
    _server.handleClient();
}

void WebServerManager::handleRoot() {
    _server.send(200, "text/html", index_html);
}

void WebServerManager::handleSend() {
    if (!_mqttClient.connected()) {
        _server.send(500, "text/plain", "Error: MQTT client not connected");
        Serial.println("[WEB_SERVER][ERROR] MQTT client not connected");
        return;
    }

    std::string deviceIP = _server.arg("ip").c_str();
    std::string command  = _server.arg("cmd").c_str();

    if (deviceIP.empty() || command.empty()) {
        _server.send(400, "text/plain", "Error: Missing parameters (ip or cmd)");
        return;
    }

    std::string message = deviceIP + ":" + command;
    _mqttClient.publish("esp32/commands", message.c_str());

    _server.send(200, "text/plain", ("Command '" + command + "' sent to " + deviceIP).c_str());
}

void WebServerManager::handleGetClients() {
    std::vector<ClientInfo> clients = _broker.getConnectedClients();

    std::string json = "{\"clients\":[";
    for (size_t i = 0; i < clients.size(); i++) {
        json += "{";
        json += "\"id\":\""        + std::string(clients[i].clientId.c_str())  + "\",";
        json += "\"ip\":\""        + std::string(clients[i].ipAddress.c_str()) + "\",";
        json += "\"connected\":"   + std::string(clients[i].connected ? "true" : "false") + ",";
        json += "\"uptime\":"      + std::to_string((millis() - clients[i].connectedTime) / 1000);
        json += "}";
        if (i < clients.size() - 1) json += ",";
    }
    json += "],\"total\":" + std::to_string(clients.size()) + "}";

    _server.send(200, "application/json", json.c_str());
}
