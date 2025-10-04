
#include "mqtt_Manager.hpp"

WiFiClient espWifi;
PubSubClient mqttClient(espWifi);

int connectToMQTTBroker(std::string mqttServer, int mqttPort = 1883) {
    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    Serial.printf("[MQTT_MANAGER][INFO] Connecting to MQTT Broker at %s:%i ...", mqttServer.c_str(), mqttPort);
    int tries = 0;
    while (!mqttClient.connected() && tries < MAX_MQTT_CONNECTION_ATTEMPTS) {
        if (mqttClient.connect("ESP32Client-EMG_Device")) {
            Serial.println("\n[MQTT_MANAGER][INFO] Connected to the MQTT Broker.");
            mqttClient.subscribe("esp32/data");
            return 1;
        } else {
            Serial.print(".");
            delay(1000);
            tries++;
        }
    }
    Serial.printf("\n[MQTT_MANAGER][ERROR] Unable to connect to the MQTT Broker after %i attempts.\n", MAX_MQTT_CONNECTION_ATTEMPTS);
    return 0;
}