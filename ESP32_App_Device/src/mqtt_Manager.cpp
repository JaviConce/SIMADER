
#include "mqtt_Manager.hpp"
#include "led_Manager.hpp"

extern LedManager ledManager;

WiFiClient espWifi;
PubSubClient mqttClient(espWifi);

MqttManager::MqttManager() {
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    std::string msg;
    
    for(int i=0; i<length; i++) msg += (char)payload[i];

    if(std::string(topic) == "esp32/commands") {
        size_t colonIndex = msg.find(':');
        if(colonIndex != std::string::npos) {

            std::string targetIP = msg.substr(0, colonIndex);
            std::string command = msg.substr(colonIndex + 1);
            std::string myIP = WiFi.localIP().toString().c_str();

            if(targetIP == myIP) {
                Serial.printf("[MQTT_MANAGER][COMMAND] Command received: %s\n", command.c_str());
                std::string response = "";

                if(command == "SendData") {
                    response = myIP + ":EMG_Data_Value_";
                    Serial.printf("[MQTT_MANAGER][RESPONSE] Sending data response: %s\n", response.c_str());
                } else if(command == "Check") {
                    ledManager.blinkNeoPixel(ORANGE_COLOR, 3, 200);
                    response = myIP + ":OK";
                    Serial.printf("[MQTT_MANAGER][RESPONSE] Sending check response: %s\n", response.c_str());
                } else {
                    Serial.printf("[MQTT_MANAGER][ERROR] Unknown command: %s\n", command.c_str());
                    return;
                }

                mqttClient.publish("esp32/responses", response.c_str());
            }
        }
    }
}

int MqttManager::connectToMQTTBroker(std::string mqttServer, int mqttPort = 1883) {
    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    mqttClient.setCallback(mqttCallback);

    Serial.printf("[MQTT_MANAGER][INFO] Connecting to MQTT Broker at %s:%i ...", mqttServer.c_str(), mqttPort);
    int tries = 0;
    while (!mqttClient.connected() && tries < MAX_MQTT_CONNECTION_ATTEMPTS) {
        if (mqttClient.connect("ESP32Client-EMG_Device")) {
            Serial.println("\n[MQTT_MANAGER][INFO] Connected to the MQTT Broker.");
            mqttClient.subscribe("esp32/commands");
            Serial.println("[MQTT_MANAGER][INFO] Subscribed to esp32/commands");
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

void MqttManager::mqttLoop() {
    mqttClient.loop();
}