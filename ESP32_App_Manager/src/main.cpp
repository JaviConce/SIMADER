#include "config.hpp"
#include "wifi_Config.hpp"
#include "mqtt_Manager.hpp"
#include "web_server_Manager.hpp"

MqttManager mqttManager;
WebServerManager webServerManager(*mqttManager.getClient(), *mqttManager.getBroker());

unsigned long lastClientPublish = 0;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);

    int ret = startWiFi_Hotspot();
    if (ret != 1) {
        Serial.println("[MAIN][ERROR] Failed to start WiFi Hotspot. Stopping execution.");
        while (true) { delay(1000); }
    }

    mqttManager.initMQTT_Server();
    xTaskCreate(taskMQTTBrokerClientManager, "MQTTBrokerClientManager", BROKER_TASK_STACK_SIZE, &mqttManager, BROKER_TASK_PRIORITY, NULL);
    mqttManager.connectBroker(WiFi.softAPIP().toString().c_str());
    webServerManager.init();
}

void loop() {
    webServerManager.handle();
    mqttManager.handleMQTTClient();

    if (millis() - lastClientPublish >= CLIENT_PUBLISH_INTERVAL_MS) {
        mqttManager.publishClients();
        lastClientPublish = millis();
    }
}
