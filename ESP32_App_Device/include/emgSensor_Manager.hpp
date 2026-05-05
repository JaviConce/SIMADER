#ifndef EMG_SENSOR_MANAGER_HPP
#define EMG_SENSOR_MANAGER_HPP

#include <Arduino.h>
#include <vector>

class EMGSensorManager {
public:
    EMGSensorManager(int pin);
    ~EMGSensorManager();

    void startSampling();
    void stopSampling();
    bool isSampling() const { return _isSampling; }

    // Calibration methods
    void startCalibration();
    bool isCalibrating() const { return _isCalibrating; }
    bool isCalibrated() const { return _calibrated; }
    std::string getCalibrationStatus();

    // Returns a summary of the data or the raw data itself (JSON string)
    std::string getProcessedDataJSON();

    // Helper for the sampling task
    void performSample();

private:
    int _pin;
    bool _isSampling;
    std::vector<int> _samples;

    // Calibration variables
    bool _calibrated;
    bool _isCalibrating;
    float _baseline;
    float _maxValue;
    int _calibrationPhase; // 0: reposo, 1: contracción máxima
    unsigned long _calibrationStartTime;
    std::vector<int> _calibrationSamples;

    TaskHandle_t _samplingTaskHandle;
    static void samplingTask(void* pvParameters);

    // Helper methods
    float normalizeValue(int rawValue);
    void processCalibrationData();
};

#endif // EMG_SENSOR_MANAGER_HPP