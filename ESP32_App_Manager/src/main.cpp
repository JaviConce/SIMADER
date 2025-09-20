
#include "wifi_Config.hpp"


void setup() {
    Serial.begin(115200);
    delay(1000);
    startWiFi_Hotspot();
}

void loop() {
}