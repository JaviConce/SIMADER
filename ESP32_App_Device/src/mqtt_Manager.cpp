#include "mqtt_Manager.hpp"
#include "led_Manager.hpp"
#include "emgSensor_Manager.hpp"

extern LedManager ledManager;
extern EMGSensorManager emgSensorManager;
extern MqttManager mqttManager;

static void blinkTaskFn(void* param) {
    uint32_t color = (uint32_t)(uintptr_t)param;
    ledManager.blinkNeoPixel(color, 0, 500);
    vTaskDelete(nullptr);
}

MqttManager::MqttManager() : mqttClient(espWifi), _sessionActive(false), _lastSendTime(0) {

}

MqttManager::~MqttManager() {

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

                if(command == "StartSession") {
                    emgSensorManager.startSampling();
                    mqttManager.startRealtimeSession();
                    response = myIP + ":EMG_Data_Sampling_Started";
                    Serial.printf("[MQTT_MANAGER][RESPONSE] Sending data response: %s\n", response.c_str());
                }else if(command == "StopSession") {
                    emgSensorManager.stopSampling();
                    mqttManager.stopRealtimeSession();
                    std::string jsonResp = emgSensorManager.getProcessedDataJSON();
                    response = myIP + ":EMG_Data_Value_" + std::string(jsonResp.c_str());
                    Serial.printf("[MQTT_MANAGER][RESPONSE] Sending data response: %s\n", response.c_str());
                }else if(command == "Calibrate") {
                    emgSensorManager.startCalibration();
                    xTaskCreate(blinkTaskFn, "blinkCalibTask", 1024, (void*)(uintptr_t)CYAN_COLOR, 1, mqttManager.getBlinkTaskHandle());
                    response = myIP + ":Calibration_Started";
                    Serial.printf("[MQTT_MANAGER][RESPONSE] Calibration started: %s\n", response.c_str());
                }else if(command == "CalibrationStatus") {
                    std::string statusJson = emgSensorManager.getCalibrationStatus();
                    if (!emgSensorManager.isCalibrating()) {
                        ledManager.stopBlink();
                        mqttManager.clearBlinkTaskHandle();
                    }
                    response = myIP + ":Calibration_Status_" + statusJson;
                    Serial.printf("[MQTT_MANAGER][RESPONSE] Calibration status: %s\n", response.c_str());
                }else if(command == "Check") {
                    ledManager.blinkNeoPixel(ORANGE_COLOR, 5, 200);
                    response = myIP + ":OK";
                    Serial.printf("[MQTT_MANAGER][RESPONSE] Sending check response: %s\n", response.c_str());
                } else {
                    Serial.printf("[MQTT_MANAGER][ERROR] Unknown command: %s\n", command.c_str());
                    return;
                }

                mqttManager.getMqttClient()->publish("esp32/responses", response.c_str());
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
            mqttClient.subscribe("esp32/commands", 1);
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

void MqttManager::startRealtimeSession() {
    _sessionActive = true;
    _lastSendTime = millis();
    xTaskCreate(blinkTaskFn, "blinkTask", 1024, (void*)(uintptr_t)GREEN_COLOR, 1, &_blinkTaskHandle);
    Serial.println("[MQTT_MANAGER][INFO] Sesión en tiempo real iniciada");
}

void MqttManager::stopRealtimeSession() {
    _sessionActive = false;
    ledManager.stopBlink();
    _blinkTaskHandle = nullptr;
    Serial.println("[MQTT_MANAGER][INFO] Sesión en tiempo real detenida");
}

void MqttManager::sendRealtimeData() {
    if (!_sessionActive) return;

    unsigned long currentTime = millis();

    // Enviar cada 500ms
    if (currentTime - _lastSendTime >= 500) {
        _lastSendTime = currentTime;

        // Obtener datos del sensor (últimas muestras)
        if (emgSensorManager.isSampling()) {
            // Leer valor actual del ADC
            int rawValue = analogRead(34);  // Pin del sensor

            // Normalizar si está calibrado
            float normalizedValue = 0.0;
            if (emgSensorManager.isCalibrated()) {
                // El valor ya está normalizado en el manager, pero necesitamos el actual
                // Por ahora usamos el raw
                normalizedValue = rawValue;
            } else {
                normalizedValue = rawValue;
            }

            // Crear JSON con los datos
            std::string json = "{";
            json += "\"emg\":" + std::to_string((int)normalizedValue) + ",";
            json += "\"raw\":" + std::to_string(rawValue) + ",";
            json += "\"ts\":" + std::to_string(currentTime);
            json += "}";

            // Publicar en esp32/emg_data
            if (mqttClient.publish("esp32/emg_data", json.c_str())) {
                Serial.printf("[MQTT_MANAGER][DATA] Enviando: %s\n", json.c_str());
            } else {
                Serial.println("[MQTT_MANAGER][ERROR] Fallo al enviar datos");
            }
        }
    }
}