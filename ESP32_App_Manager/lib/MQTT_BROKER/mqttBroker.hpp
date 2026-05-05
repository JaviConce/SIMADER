#ifndef ESP32MQTTBroker_h
#define ESP32MQTTBroker_h

#include <sMQTTBroker.h>
#include <vector>
#include <string>
#include <map>

struct ClientInfo {
    std::string clientId;
    std::string ipAddress;
    bool connected;
    unsigned long connectedTime;
};

class MQTTBroker: public sMQTTBroker {
public:
    MQTTBroker();
    void begin(unsigned short mqttPort = 1883);
    void loop();
    std::vector<ClientInfo> getConnectedClients();

protected:
    virtual bool onEvent(sMQTTEvent *event) override;
    unsigned long lastMemoryCheckTime;
    unsigned long freeRam;

private:
    std::map<std::string, ClientInfo> connectedClients;
};

#endif