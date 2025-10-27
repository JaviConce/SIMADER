#include <Arduino.h>
#include "wifi_Config.hpp"
#include "mqtt_Manager.hpp"
#include "led_Manager.hpp"

const int sensorPin = 34;

LedManager ledManager;
WifiConfig wifiConfig;
MqttManager mqttManager;


void capture_data(){
  int rawValue = analogRead(sensorPin);

  float voltage = (rawValue / 4095.0) * 3.3;
  
  Serial.print("Raw: ");
  Serial.print(rawValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage);
  Serial.println("V");
  
  delay(10); // 100Hz de muestreo

}

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);
  analogReadResolution(12);

  ledManager.initNeoPixel();

  Serial.begin(115200);
  delay(1000);
  int ret = wifiConfig.startWIFI_CONFIG();
  if (ret != 1) {
    Serial.println("[MAIN][ERROR] Failed to connect to WiFi.");
    return;
  }
  Serial.println("[MAIN][INFO] WiFi connected successfully.");
  Serial.printf("[MAIN][INFO] Local IP: %s\n", WiFi.localIP().toString().c_str());
  mqttManager.connectToMQTTBroker(WiFi.gatewayIP().toString().c_str(), 1883);
}

void loop() {
  //capture_data();  TODO: review this method
  mqttManager.mqttLoop();
  delay(10); // Pequeño delay para no saturar el CPU
}