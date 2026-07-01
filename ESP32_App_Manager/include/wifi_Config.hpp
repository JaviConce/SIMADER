#include <WiFi.h>
#include <string>
#include "config.hpp"

// Global configuration variables
const std::string ssid = WIFI_SSID;
const std::string password = WIFI_PASSWORD;

const int MAX_CREATION_ATTEMPTS = WIFI_MAX_ATTEMPTS;

const IPAddress app_staticIP(WIFI_STATIC_IP);
const IPAddress app_gateway(WIFI_GATEWAY);
const IPAddress app_subnet(WIFI_SUBNET);

// Functions declarations
int startWiFi_Hotspot();
