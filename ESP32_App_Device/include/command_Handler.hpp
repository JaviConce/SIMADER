#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <string>

class EMGSensorManager;
class LedManager;
class MqttManager;

class CommandHandler {
public:
    CommandHandler(EMGSensorManager& emg, LedManager& led, MqttManager& mqtt);

    // Procesa un comando dirigido a este dispositivo y devuelve la respuesta
    std::string handle(const std::string& command);

private:
    EMGSensorManager& _emg;
    LedManager& _led;
    MqttManager& _mqtt;

    std::string onStartSession();
    std::string onStopSession();
    std::string onCalibrate();
    std::string onCancelCalibration();
    std::string onCalibrationStatus();
    std::string onCheck();
};

#endif // COMMAND_HANDLER_HPP
