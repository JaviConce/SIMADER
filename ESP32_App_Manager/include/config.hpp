#ifndef CONFIG_HPP
#define CONFIG_HPP

// ===================================
// WIFI HOTSPOT
// ===================================
#define WIFI_SSID               "Manager_app_V1"
#define WIFI_PASSWORD           "Manager_app_V1"
#define WIFI_MAX_ATTEMPTS       3
#define WIFI_STATIC_IP          192, 168, 4, 1
#define WIFI_GATEWAY            192, 168, 4, 1
#define WIFI_SUBNET             255, 255, 255, 0

// ===================================
// MQTT
// ===================================
#define MQTT_PORT               1883
#define MQTT_CLIENT_ID          "ESP32ManagerClient"
#define MQTT_BUFFER_SIZE        512

// ===================================
// PUBLICACIÓN DE CLIENTES
// ===================================
#define CLIENT_PUBLISH_INTERVAL_MS  5000    // Publicar lista de clientes cada 5 segundos

// ===================================
// TAREAS FREERTOS
// ===================================
#define BROKER_TASK_STACK_SIZE  4096
#define BROKER_TASK_PRIORITY    3

// ===================================
// SERIAL
// ===================================
#define SERIAL_BAUD_RATE        115200

#endif // CONFIG_HPP
