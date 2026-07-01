#ifndef MQTT_MANAGER_HPP
#define MQTT_MANAGER_HPP

#include <string>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.hpp"

class CommandHandler;

class MqttManager {
public:
    MqttManager();
    ~MqttManager();

    void setCommandHandler(CommandHandler* handler) { _commandHandler = handler; }

    int connectToMQTTBroker(std::string mqttServer, int mqttPort = MQTT_PORT);
    void mqttLoop();
    PubSubClient* getMqttClient() { return &mqttClient; }
    TaskHandle_t* getBlinkTaskHandle() { return &_blinkTaskHandle; }
    void clearBlinkTaskHandle() { _blinkTaskHandle = nullptr; }

    void startRealtimeSession();
    void stopRealtimeSession();
    void sendRealtimeData();
    bool isSessionActive() const { return _sessionActive; }

private:
    WiFiClient espWifi;
    PubSubClient mqttClient;
    CommandHandler* _commandHandler = nullptr;

    bool _sessionActive;
    unsigned long _lastSendTime;
    TaskHandle_t _blinkTaskHandle = nullptr;

    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

#endif // MQTT_MANAGER_HPP
