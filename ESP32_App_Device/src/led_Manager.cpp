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

void LedManager::blinkNeoPixel(uint32_t color, int times, int delayMs) {
    for(int i = 0; i < times; i++) {
        neoPixel.setPixelColor(0, color);
        neoPixel.show();
        delay(delayMs);
        neoPixel.setPixelColor(0, 0); // Apagar
        neoPixel.show();
        delay(delayMs);
    }
}