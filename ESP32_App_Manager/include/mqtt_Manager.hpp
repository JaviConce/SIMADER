#ifndef MQTT_MANAGER_HPP
#define MQTT_MANAGER_HPP

#include "mqttBroker.hpp"
#include "config.hpp"
#include <string>
#include <WiFi.h>
#include <PubSubClient.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class MqttManager {
public:
    MqttManager();
    ~MqttManager();

    int initMQTT_Server();
    void connectBroker(std::string mqttBroker);
    void handleMQTTClient();
    void publishClients();

    MQTTBroker* getBroker()   { return &broker; }
    PubSubClient* getClient() { return &client; }

private:
    MQTTBroker broker;
    WiFiClient wifiClient;
    PubSubClient client;

    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

// Tarea FreeRTOS para el broker MQTT
void taskMQTTBrokerClientManager(void *parameter);

#endif // MQTT_MANAGER_HPP
