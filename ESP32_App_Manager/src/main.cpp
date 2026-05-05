#include "wifi_Config.hpp"
#include "mqtt_Manager.hpp"

MqttManager mqttManager;

unsigned long lastClientPublish = 0;
const unsigned long CLIENT_PUBLISH_INTERVAL = 5000; // Publicar cada 5 segundos

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

    mqttManager.initMQTT_Server();
    xTaskCreate(taskMQTTBrokerClientManager, "MQTTBrokerClientManager", 4096, &mqttManager, 3, NULL);
    mqttManager.connectBroker(WiFi.softAPIP().toString().c_str());
    mqttManager.initWebServer();
}

void loop() {
    mqttManager.handleWebServer();
    mqttManager.handleMQTTClient();

    // Publicar lista de clientes conectados periódicamente
    if(millis() - lastClientPublish >= CLIENT_PUBLISH_INTERVAL) {
        mqttManager.publishClients();
        lastClientPublish = millis();
    }
}