#include <Arduino.h>
#include "wifi_Config.hpp"

void setup() {
  Serial.begin(115200);
  delay(1000);
  int ret = startWIFI_CONFIG();
  if (ret != 1) {
    Serial.println("[MAIN][ERROR] Failed to connect to WiFi.");
  } else {
    Serial.println("[MAIN][INFO] WiFi connected successfully.");
  }
}

void loop() {
}