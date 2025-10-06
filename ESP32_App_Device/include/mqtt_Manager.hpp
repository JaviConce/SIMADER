
#include <string>
#include <WiFi.h>
#include <PubSubClient.h>

const int MAX_MQTT_CONNECTION_ATTEMPTS = 10;

int connectToMQTTBroker(std::string mqttServer, int mqttPort);
void mqttLoop();
void mqttCallback(char* topic, byte* payload, unsigned int length);