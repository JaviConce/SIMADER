#include <Arduino.h>
#include "wifi_Config.hpp"
#include "mqtt_Manager.hpp"

void setup() {
  Serial.begin(115200);
  delay(1000);
  int ret = startWIFI_CONFIG();
  if (ret != 1) {
    Serial.println("[MAIN][ERROR] Failed to connect to WiFi.");
    return;
  }
  Serial.println("[MAIN][INFO] WiFi connected successfully.");
  connectToMQTTBroker(WiFi.softAPIP().toString().c_str(), 1883);
}

void loop() {
}