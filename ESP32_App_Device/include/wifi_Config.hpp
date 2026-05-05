
#include <WiFi.h>

const std::string ssid = "Manager_app_V1"; // Replace with your network SSID
const std::string password = "Manager_app_V1"; // Replace with your network password
const int MAX_CONNECTION_ATTEMPTS = 10;

int startWIFI_CONFIG();

class WifiConfig {
public:
    WifiConfig();
    int startWIFI_CONFIG();
private:
};