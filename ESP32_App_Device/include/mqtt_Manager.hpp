
#include <string>
#include <WiFi.h>
#include <PubSubClient.h>

const int MAX_MQTT_CONNECTION_ATTEMPTS = 10;


class MqttManager {
public:
    MqttManager();
    int connectToMQTTBroker(std::string mqttServer, int mqttPort);
    void mqttLoop();

private:
};