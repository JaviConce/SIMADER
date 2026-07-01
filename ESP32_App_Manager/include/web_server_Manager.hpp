#ifndef WEB_SERVER_MANAGER_HPP
#define WEB_SERVER_MANAGER_HPP

#include <WebServer.h>
#include <PubSubClient.h>
#include <string>

class MQTTBroker;

class WebServerManager {
public:
    WebServerManager(PubSubClient& mqttClient, MQTTBroker& broker);

    void init();
    void handle();

private:
    WebServer _server;
    PubSubClient& _mqttClient;
    MQTTBroker& _broker;

    void handleRoot();
    void handleSend();
    void handleGetClients();
};

#endif // WEB_SERVER_MANAGER_HPP
