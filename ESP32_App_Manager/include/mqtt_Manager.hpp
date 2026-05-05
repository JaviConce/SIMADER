#ifndef MQTT_MANAGER_HPP
#define MQTT_MANAGER_HPP

#include "mqttBroker.hpp"
#include <string>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class MqttManager {
public:
    MqttManager();
    ~MqttManager();

    int initMQTT_Server();
    void connectBroker(std::string mqttBroker);
    void initWebServer();
    void handleWebServer();
    void handleMQTTClient();
    void publishClients();

    MQTTBroker* getBroker() { return &broker; }
    PubSubClient* getClient() { return &client; }
    WebServer* getServer() { return &server; }

private:
    static const int mqttPort = 1883;
    MQTTBroker broker;
    WiFiClient wifiClient;
    PubSubClient client;
    WebServer server;

    // Handlers privados
    void handleRoot();
    void handleSend();
    void handleGetClients();

    // Callback para MQTT
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

// Tarea para el broker MQTT
void taskMQTTBrokerClientManager(void *parameter);

#endif // MQTT_MANAGER_HPP





