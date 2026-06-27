#include "led_Manager.hpp"
Adafruit_NeoPixel neoPixel(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

LedManager::LedManager() {
    // Constructor vacío
}

void LedManager::initNeoPixel() {
    neoPixel.begin();
    neoPixel.setBrightness(50); // Brillo medio (0-255)
    neoPixel.show(); // Apagar el LED inicialmente
}

// Si times == 0 parpadea indefinidamente hasta que se llame stopBlink()
void LedManager::blinkNeoPixel(uint32_t color, int times, int delayMs) {
    _blinking = true;
    int count = 0;
    while (_blinking && (times == 0 || count < times)) {
        neoPixel.setPixelColor(0, color);
        neoPixel.show();
        delay(delayMs);
        neoPixel.setPixelColor(0, 0);
        neoPixel.show();
        delay(delayMs);
        if (times > 0) count++;
    }
    neoPixel.setPixelColor(0, 0);
    neoPixel.show();
}

void LedManager::stopBlink() {
    _blinking = false;
}