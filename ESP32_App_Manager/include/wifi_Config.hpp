#include <WiFi.h>
#include <string>
#include <vector>

// Global configuration variables
const std::string ssid = "Manager_app_V1";      
const std::string password = "Manager_app_V1";  //TODO: Implement secure password management

const int MAX_CREATION_ATTEMPTS = 3;

const IPAddress app_staticIP(192, 168, 4, 1);
const IPAddress app_gateway(192, 168, 4, 1);
const IPAddress app_subnet(255, 255, 255, 0);

// Functions declarations
int startWiFi_Hotspot();

struct ClientAttemps {
    std::string mac;
    int attempts;
};
