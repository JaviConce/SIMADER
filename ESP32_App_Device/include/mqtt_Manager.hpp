#ifndef MQTT_MANAGER_HPP
#define MQTT_MANAGER_HPP

#include <string>
#include <WiFi.h>
#include <PubSubClient.h>

const int MAX_MQTT_CONNECTION_ATTEMPTS = 10;

class MqttManager {
public:
    MqttManager();
    ~MqttManager();
    int connectToMQTTBroker(std::string mqttServer, int mqttPort);
    void mqttLoop();
    PubSubClient* getMqttClient() { return &mqttClient; }
    TaskHandle_t* getBlinkTaskHandle() { return &_blinkTaskHandle; }
    void clearBlinkTaskHandle() { _blinkTaskHandle = nullptr; }

    // Métodos para sesión en tiempo real
    void startRealtimeSession();
    void stopRealtimeSession();
    void sendRealtimeData();
    bool isSessionActive() const { return _sessionActive; }

private:
    const int mqttPort = 1883;
    WiFiClient espWifi;
    PubSubClient mqttClient;

    // Estado de sesión en tiempo real
    bool _sessionActive;
    unsigned long _lastSendTime;
    TaskHandle_t _blinkTaskHandle = nullptr;
};

#endif // MQTT_MANAGER_HPP