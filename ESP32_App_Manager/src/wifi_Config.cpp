
#include "wifi_Config.hpp"

std::vector<ClientAttemps> clientList;


void onStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
        info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
        info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
        info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
    
    Serial.printf("[WIFI_CONFIG][INFO] Device Connected - MAC: %s\n", macStr);
}

void onStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
        info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],
        info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],
        info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5]);
    
    Serial.printf("[WIFI_CONFIG][INFO] Device Disconnected - MAC: %s\n", macStr);
}

int startWiFi_Hotspot() {
    WiFi.mode(WIFI_AP);
    
    WiFi.onEvent(onStationConnected, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    WiFi.onEvent(onStationDisconnected, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

    int tries = 0;
    bool create_success = false;
    if(!WiFi.softAPConfig(app_staticIP, app_gateway, app_subnet)) {
        Serial.println("[WIFI_CONFIG][ERROR] Error setting static IP configuration.");
        return 0;
    }
    while (!create_success && tries < MAX_CREATION_ATTEMPTS) {
        if(WiFi.softAP(ssid.c_str(), password.c_str())) {
            Serial.println("\n[WIFI_CONFIG][INFO] Wifi Access Point created successfully.");
            Serial.printf("[WIFI_CONFIG][INFO] SSID: %s\n", ssid.c_str());
            Serial.printf("[WIFI_CONFIG][INFO] APP IP address: %s\n", WiFi.softAPIP().toString().c_str());
            create_success = true;
            return 1;
        } else {
            Serial.println("[WIFI_CONFIG][ERROR] Error creating WiFi Access Point.");
            if(tries < MAX_CREATION_ATTEMPTS) {
                Serial.println("[WIFI_CONFIG][INFO] Retrying in 2 seconds.");
                delay(2000);
            }
        }
        tries++;
    }
    if (!create_success) {
        Serial.printf("\n[WIFI_CONFIG][ERROR] Error creating the WiFi access point after %i attempts .\n", MAX_CREATION_ATTEMPTS);
        return 0;
    }
    return 0;
}

