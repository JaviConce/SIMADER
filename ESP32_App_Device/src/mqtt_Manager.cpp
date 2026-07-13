#include "mqtt_Manager.hpp"
#include "command_Handler.hpp"
#include "led_Manager.hpp"
#include "emgSensor_Manager.hpp"
#include "config.hpp"

extern LedManager ledManager;
extern EMGSensorManager emgSensorManager;
extern MqttManager mqttManager;

// ===================================
// TAREA FREERTOS PARA PARPADEO LED
// ===================================

static void blinkTaskFn(void* param) {
    uint32_t color = (uint32_t)(uintptr_t)param;
    ledManager.blinkNeoPixel(color, 0, LED_BLINK_INTERVAL_MS);
    vTaskDelete(nullptr);
}

// ===================================
// CONSTRUCTOR / DESTRUCTOR
// ===================================

MqttManager::MqttManager() : mqttClient(espWifi), _sessionActive(false), _lastSendTime(0) {}

MqttManager::~MqttManager() {}

// ===================================
// CALLBACK MQTT — solo parsea y delega
// ===================================

void MqttManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
    std::string msg;
    for (int i = 0; i < (int)length; i++) msg += (char)payload[i];

    if (std::string(topic) != "esp32/commands") return;

    size_t colonIndex = msg.find(':');
    if (colonIndex == std::string::npos) return;

    std::string targetIP = msg.substr(0, colonIndex);
    std::string command  = msg.substr(colonIndex + 1);
    std::string myIP     = WiFi.localIP().toString().c_str();

    if (targetIP != myIP) return;

    Serial.printf("[MQTT_MANAGER][COMMAND] Received: %s\n", command.c_str());

    if (!mqttManager._commandHandler) {
        Serial.println("[MQTT_MANAGER][ERROR] No CommandHandler set");
        return;
    }

    std::string result = mqttManager._commandHandler->handle(command);
    if (result.empty()) return;

    std::string response = myIP + ":" + result;
    mqttManager.getMqttClient()->publish("esp32/responses", response.c_str());
}

// ===================================
// CONEXIÓN
// ===================================

int MqttManager::connectToMQTTBroker(std::string mqttServer, int mqttPort) {
    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    mqttClient.setCallback(MqttManager::mqttCallback);

    Serial.printf("[MQTT_MANAGER][INFO] Connecting to MQTT Broker at %s:%i ...", mqttServer.c_str(), mqttPort);
    int tries = 0;
    while (!mqttClient.connected() && tries < MAX_MQTT_CONNECTION_ATTEMPTS) {
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println("\n[MQTT_MANAGER][INFO] Connected to the MQTT Broker.");
            mqttClient.subscribe("esp32/commands", 1);
            Serial.println("[MQTT_MANAGER][INFO] Subscribed to esp32/commands");
            return 1;
        }
        Serial.print(".");
        delay(1000);
        tries++;
    }
    Serial.printf("\n[MQTT_MANAGER][ERROR] Unable to connect after %i attempts.\n", MAX_MQTT_CONNECTION_ATTEMPTS);
    return 0;
}

void MqttManager::mqttLoop() {
    mqttClient.loop();
}

// ===================================
// GESTIÓN DE SESIÓN
// ===================================

void MqttManager::startRealtimeSession() {
    _sessionActive = true;
    _lastSendTime = millis();
    xTaskCreatePinnedToCore(blinkTaskFn, "blinkTask", 1024, (void*)(uintptr_t)GREEN_COLOR, 1, &_blinkTaskHandle, 1);
    Serial.println("[MQTT_MANAGER][INFO] Sesión en tiempo real iniciada");
}

void MqttManager::stopRealtimeSession() {
    _sessionActive = false;
    ledManager.stopBlink();
    _blinkTaskHandle = nullptr;
    Serial.println("[MQTT_MANAGER][INFO] Sesión en tiempo real detenida");
}

// ===================================
// ENVÍO DE DATOS EN TIEMPO REAL
// ===================================

void MqttManager::sendRealtimeData() {
    if (!_sessionActive) return;

    unsigned long currentTime = millis();
    if (currentTime - _lastSendTime < EMG_REALTIME_INTERVAL_MS) return;
    _lastSendTime = currentTime;

    if (!emgSensorManager.isSampling()) return;

    int rawValue       = analogRead(EMG_SENSOR_PIN);
    float emgValue     = emgSensorManager.getCurrentValue();
    float voltage      = (rawValue / 4095.0f) * 3.3f;

    char vBuf[16];
    snprintf(vBuf, sizeof(vBuf), "%.4f", voltage);

    std::string json = "{";
    json += "\"emg\":"     + std::to_string(emgValue) + ",";
    json += "\"raw\":"     + std::to_string(rawValue)  + ",";
    json += "\"voltaje\":" + std::string(vBuf)          + ",";
    json += "\"ts\":"      + std::to_string(currentTime);
    json += "}";

    if (mqttClient.publish("esp32/emg_data", json.c_str())) {
        Serial.printf("[MQTT_MANAGER][DATA] Enviando: %s\n", json.c_str());
    } else {
        Serial.println("[MQTT_MANAGER][ERROR] Fallo al enviar datos");
    }
}
