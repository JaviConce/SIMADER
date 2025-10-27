
#include <Adafruit_NeoPixel.h>

#define CYAN_COLOR    0x00FFFF
#define GREEN_COLOR   0x00FF00
#define RED_COLOR     0xFF0000
#define YELLOW_COLOR  0xFFFF00
#define ORANGE_COLOR  0xFFA500
#define WHITE_COLOR   0xFFFFFF

#define NEO_BRIGTHNESS 10

// NeoPixel configuration for Adafruit Feather ESP32 V2
#define NUM_PIXELS 1



class LedManager {
public:

    LedManager();
    void initNeoPixel();
    void blinkNeoPixel(uint32_t color, int times, int delayMs);

};