#include "wifi_Config.hpp"
#include "mqtt_Manager.hpp"



void setup() {
    Serial.begin(115200);
    delay(1000);
    int ret = startWiFi_Hotspot();
    if(ret != 1) {
        Serial.println("[MAIN][ERROR] Failed to start WiFi Hotspot. Stopping execution.");
        while(true) {
            delay(1000);
        }
    }
    initMQTT_Server();
    xTaskCreate(taskMQTTBrokerClientManager, "MQTTBrokerClientManager", 4096, NULL, 3, NULL);
    connectBroker(WiFi.softAPIP().toString().c_str());
    initWebServer();
}

void loop() {
    handleWebServer();
    handleMQTTClient();
}