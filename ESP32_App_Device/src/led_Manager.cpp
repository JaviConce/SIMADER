#include "led_Manager.hpp"
#include "config.hpp"
Adafruit_NeoPixel neoPixel(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

LedManager::LedManager() {
    // Constructor vacío
}

void LedManager::initNeoPixel() {
    neoPixel.begin();
    neoPixel.setBrightness(LED_BRIGHTNESS);
    neoPixel.show(); // Apagar el LED inicialmente
}

// Si times == 0 parpadea indefinidamente hasta que se llame stopBlink()
void LedManager::blinkNeoPixel(uint32_t color, int times, int delayMs) {
    _blinking = true;
    int count = 0;
    while (_blinking && (times == 0 || count < times)) {
        neoPixel.setPixelColor(0, color);
        neoPixel.show();
        vTaskDelay(pdMS_TO_TICKS(delayMs));
        neoPixel.setPixelColor(0, 0);
        neoPixel.show();
        vTaskDelay(pdMS_TO_TICKS(delayMs));
        if (times > 0) count++;
    }
    neoPixel.setPixelColor(0, 0);
    neoPixel.show();
}

void LedManager::stopBlink() {
    _blinking = false;
}