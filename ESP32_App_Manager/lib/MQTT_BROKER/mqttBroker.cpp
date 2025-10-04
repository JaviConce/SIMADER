#include "mqttBroker.hpp"

MQTTBroker::MQTTBroker() {

}

void MQTTBroker::begin(unsigned short mqttPort) {
    Serial.println("[MQTTBroker][INFO] Starting MQTT Broker...");
    
    if(!init(mqttPort))
        Serial.println("[MQTTBroker][ERROR] Failed to start MQTT Broker...");
    
}

void MQTTBroker::loop() {
    update();
}

bool MQTTBroker::onEvent(sMQTTEvent *event) {
    switch(event->Type()) {
    case NewClient_sMQTTEventType:
        {
        sMQTTNewClientEvent *e = (sMQTTNewClientEvent*)event;
        sMQTTClient *client = e->Client();
        std::string clientId = client->getClientId();

        Serial.print("[MQTTBroker][INFO] New client connected with ID: ");
        Serial.println(clientId.c_str());
        }
        break;
    case LostConnect_sMQTTEventType:
        Serial.println("[MQTTBroker][INFO] Broker lost connection, reconnecting WiFi...");
        WiFi.reconnect();
        break;
    case Subscribe_sMQTTEventType:
        {
        sMQTTSubUnSubClientEvent *e = (sMQTTSubUnSubClientEvent*)event;
        sMQTTClient *client = e->Client();
        std::string topic = e->Topic();
        Serial.printf("[MQTTBroker][INFO] Client: %s subscribed a topic %s\n",client->getClientId().c_str(),topic.c_str());
        break;
        }

    case UnSubscribe_sMQTTEventType:
        Serial.println("[MQTTBroker][INFO] Client unsubscribed from a topic");
        break;
    }
    return true;
}