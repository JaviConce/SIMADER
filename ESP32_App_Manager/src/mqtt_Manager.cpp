#include "mqtt_Manager.hpp"
#include <Arduino.h>

MQTTBroker broker;
WiFiClient wifiClient;
PubSubClient client(wifiClient);


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
    String msg;
    for(int i=0; i<length; i++) msg += (char)payload[i];
    Serial.printf("[MQTT_MANAGER][MESSAGE] Message received from topic %s: %s\n", topic, msg.c_str());
}

void connectBroker(std::string mqttBroker) {
    client.setServer(mqttBroker.c_str(), mqttPort);
    client.setCallback(callback);

    while(!client.connected()){
        Serial.print("[MQTT_MANAGER][INFO] Connecting to the MQTT broker...\n");
        if(client.connect("ESP32ManagerClient")){
            // Suscribirse al topic de solicitudes del servidor
            client.subscribe("esp32/data");
        } else {
            Serial.printf("[MQTT_MANAGER][ERROR] Failure, rc=%d. Retrying in 2 seconds...\n", client.state());
            delay(2000);
        }
    }
}