#include <Arduino.h>
#include "config.hpp"
#include "wifi_Config.hpp"
#include "mqtt_Manager.hpp"
#include "led_Manager.hpp"
#include "emgSensor_Manager.hpp"
#include "command_Handler.hpp"

LedManager ledManager;
WifiConfig wifiConfig;
MqttManager mqttManager;
EMGSensorManager emgSensorManager(EMG_SENSOR_PIN);
CommandHandler commandHandler(emgSensorManager, ledManager, mqttManager);

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  pinMode(EMG_SENSOR_PIN, INPUT);
  analogReadResolution(EMG_ADC_RESOLUTION);

  ledManager.initNeoPixel();

  delay(1000);
  int ret = wifiConfig.startWIFI_CONFIG();
  if (ret != 1) {
    Serial.println("[MAIN][ERROR] Failed to connect to WiFi.");
    return;
  }
  Serial.println("[MAIN][INFO] WiFi connected successfully.");
  Serial.printf("[MAIN][INFO] Local IP: %s\n", WiFi.localIP().toString().c_str());
  mqttManager.setCommandHandler(&commandHandler);
  mqttManager.connectToMQTTBroker(WiFi.gatewayIP().toString().c_str(), MQTT_PORT);
}

void loop() {
  mqttManager.mqttLoop();
  mqttManager.sendRealtimeData();
  delay(EMG_SAMPLING_RATE_MS);
}