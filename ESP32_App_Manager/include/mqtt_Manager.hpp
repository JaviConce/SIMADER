#pragma once
#include "mqttBroker.hpp"

#include <string>
#include <WiFi.h>
#include <PubSubClient.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const int mqttPort = 1883; 

int initMQTT_Server();
void taskMQTTBrokerClientManager(void *parameter);
void connectBroker(std::string mqttBroker);

