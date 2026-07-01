
#include <WiFi.h>
#include "config.hpp"

const std::string ssid = WIFI_SSID;
const std::string password = WIFI_PASSWORD;
const int MAX_CONNECTION_ATTEMPTS = WIFI_MAX_ATTEMPTS;

int startWIFI_CONFIG();

class WifiConfig {
public:
    WifiConfig();
    int startWIFI_CONFIG();
private:
};