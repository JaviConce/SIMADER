#include "wifi_Config.hpp"

int startWIFI_CONFIG() {
    WiFi.mode(WIFI_STA);
    Serial.printf("[WIFI_CONFIG][INFO] Connecting to SSID: %s.", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < MAX_CONNECTION_ATTEMPTS) {
        delay(1000);
        Serial.print(".");
        tries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI_CONFIG][INFO] Successful connection to the Access Point.");
        Serial.printf("[WIFI_CONFIG][INFO] IP address: %s\n", WiFi.localIP().toString().c_str());
        return 1;
    } else {
        Serial.printf("\n[WIFI_CONFIG][ERROR] Unable to connect to the AP after %i attempts.\n", MAX_CONNECTION_ATTEMPTS);
        return 0;
    }
}
