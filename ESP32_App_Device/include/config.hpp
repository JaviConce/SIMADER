#ifndef CONFIG_HPP
#define CONFIG_HPP

// ===================================
// WIFI
// ===================================
#define WIFI_SSID           "Manager_app_V1"
#define WIFI_PASSWORD       "Manager_app_V1"
#define WIFI_MAX_ATTEMPTS   10

// ===================================
// MQTT
// ===================================
#define MQTT_PORT                    1883
#define MQTT_CLIENT_ID               "ESP32Client-EMG_Device"
#define MAX_MQTT_CONNECTION_ATTEMPTS 10

// ===================================
// SENSOR EMG
// ===================================
#define EMG_SENSOR_PIN      34
#define EMG_ADC_RESOLUTION  12
#define EMG_ADC_MAX_VALUE   4095.0f
#define EMG_SAMPLING_RATE_MS    10      // 100 Hz
#define EMG_REALTIME_INTERVAL_MS 200    // Envío en tiempo real cada 200ms

// ===================================
// CALIBRACIÓN
// ===================================
#define CALIB_PHASE0_DURATION_MS    10000   // Fase reposo: 10 segundos
#define CALIB_PHASE1_DURATION_MS    5000    // Fase contracción: 5 segundos
#define CALIB_MIN_RANGE             100     // Diferencia mínima válida entre reposo y máximo

// ===================================
// LED
// ===================================
#define LED_BLINK_INTERVAL_MS       500     // Intervalo de parpadeo durante sesión/calibración
#define LED_CHECK_BLINKS            5
#define LED_CHECK_INTERVAL_MS       200
#define LED_BRIGHTNESS              50      // 0-255

// ===================================
// SERIAL
// ===================================
#define SERIAL_BAUD_RATE    115200

#endif // CONFIG_HPP
