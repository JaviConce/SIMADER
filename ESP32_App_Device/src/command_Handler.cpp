#include "command_Handler.hpp"
#include "emgSensor_Manager.hpp"
#include "led_Manager.hpp"
#include "mqtt_Manager.hpp"
#include "config.hpp"
#include <Arduino.h>

CommandHandler::CommandHandler(EMGSensorManager& emg, LedManager& led, MqttManager& mqtt)
    : _emg(emg), _led(led), _mqtt(mqtt) {}

std::string CommandHandler::handle(const std::string& command) {
    if (command == "StartSession")        return onStartSession();
    if (command == "StopSession")         return onStopSession();
    if (command == "Calibrate")           return onCalibrate();
    if (command == "CancelCalibration")   return onCancelCalibration();
    if (command == "CalibrationStatus")   return onCalibrationStatus();
    if (command == "Check")               return onCheck();

    Serial.printf("[COMMAND_HANDLER][ERROR] Unknown command: %s\n", command.c_str());
    return "";
}

std::string CommandHandler::onStartSession() {
    _emg.startSampling();
    _mqtt.startRealtimeSession();
    Serial.println("[COMMAND_HANDLER][INFO] Session started");
    return "EMG_Data_Sampling_Started";
}

std::string CommandHandler::onStopSession() {
    _emg.stopSampling();
    _mqtt.stopRealtimeSession();
    std::string jsonResp = _emg.getProcessedDataJSON();
    Serial.println("[COMMAND_HANDLER][INFO] Session stopped");
    return "EMG_Data_Value_" + jsonResp;
}

std::string CommandHandler::onCalibrate() {
    _emg.startCalibration();
    xTaskCreatePinnedToCore(
        [](void* param) {
            LedManager* led = static_cast<LedManager*>(param);
            led->blinkNeoPixel(CYAN_COLOR, 0, LED_BLINK_INTERVAL_MS);
            vTaskDelete(nullptr);
        },
        "blinkCalibTask", 1024, &_led, 1, _mqtt.getBlinkTaskHandle(), 1
    );
    Serial.println("[COMMAND_HANDLER][INFO] Calibration started");
    return "Calibration_Started";
}

std::string CommandHandler::onCancelCalibration() {
    _emg.cancelCalibration();
    _led.stopBlink();
    _mqtt.clearBlinkTaskHandle();
    Serial.println("[COMMAND_HANDLER][INFO] Calibration cancelled");
    return "Calibration_Cancelled";
}

std::string CommandHandler::onCalibrationStatus() {
    std::string statusJson = _emg.getCalibrationStatus();
    if (!_emg.isCalibrating()) {
        _led.stopBlink();
        _mqtt.clearBlinkTaskHandle();
    }
    return "Calibration_Status_" + statusJson;
}

std::string CommandHandler::onCheck() {
    _led.blinkNeoPixel(ORANGE_COLOR, LED_CHECK_BLINKS, LED_CHECK_INTERVAL_MS);
    Serial.println("[COMMAND_HANDLER][INFO] Check OK");
    return "OK";
}
