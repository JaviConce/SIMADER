#pragma once
#include "mqttBroker.hpp"

#include <string>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const int mqttPort = 1883;

extern MQTTBroker broker;

int initMQTT_Server();
void taskMQTTBrokerClientManager(void *parameter);
void connectBroker(std::string mqttBroker);
void initWebServer();
void handleWebServer();
void handleMQTTClient();

