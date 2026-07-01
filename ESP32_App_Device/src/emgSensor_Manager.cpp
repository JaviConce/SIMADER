#include "emgSensor_Manager.hpp"
#include "config.hpp"

EMGSensorManager::EMGSensorManager(int pin)
    : _pin(pin),
      _isSampling(false),
      _samplingTaskHandle(NULL),
      _calibrated(false),
      _isCalibrating(false),
      _baseline(0.0),
      _maxValue(EMG_ADC_MAX_VALUE),
      _calibrationPhase(0),
      _calibrationStartTime(0) {
    pinMode(_pin, INPUT);
}

EMGSensorManager::~EMGSensorManager() {
    stopSampling();
}

void EMGSensorManager::startSampling() {
    if (_isSampling) return;
    
    _samples.clear();
    _isSampling = true;
    
    // Create a task for sampling on core 1 (Arduino loop usually runs on core 1, but we want high priority)
    xTaskCreatePinnedToCore(
        this->samplingTask,
        "EMGSamplingTask",
        4096,
        this,
        10, // High priority
        &_samplingTaskHandle,
        1
    );
    
    Serial.println("[EMG_MANAGER] Sampling started.");
}

void EMGSensorManager::stopSampling() {
    if (!_isSampling) return;
    
    _isSampling = false;
    if (_samplingTaskHandle != NULL) {
        vTaskDelete(_samplingTaskHandle);
        _samplingTaskHandle = NULL;
    }
    
    Serial.printf("[EMG_MANAGER] Sampling stopped. Captured %d samples.\n", (int)_samples.size());
}

void EMGSensorManager::performSample() {
    int rawValue = analogRead(_pin);

    // If calibrating, store in calibration samples
    if (_isCalibrating) {
        _calibrationSamples.push_back(rawValue);
    } else {
        // Normal sampling - store normalized value if calibrated
        if (_calibrated) {
            float normalized = normalizeValue(rawValue);
            _samples.push_back((int)(normalized * 100)); // Store as percentage (0-100)
        } else {
            _samples.push_back(rawValue); // Store raw value if not calibrated
        }
    }
}

void EMGSensorManager::samplingTask(void* pvParameters) {
    EMGSensorManager* manager = (EMGSensorManager*)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // 100Hz

    for (;;) {
        manager->performSample();

        // Process calibration if in calibration mode
        if (manager->_isCalibrating) {
            manager->processCalibrationData();
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

std::string EMGSensorManager::getProcessedDataJSON() {
    if (_samples.empty()) return "{\"status\":\"no_data\"}";

    long sum = 0;
    for (int s : _samples) sum += s;
    float avg = (float)sum / _samples.size();

    std::string json = "{";
    json += "\"samples\":" + std::to_string((int)_samples.size()) + ",";
    json += "\"average\":" + std::to_string(avg) + ",";
    json += "\"calibrated\":" + std::string(_calibrated ? "true" : "false");
    json += "}";

    return json;
}

// ===================================
// CALIBRATION METHODS
// ===================================

void EMGSensorManager::startCalibration() {
    if (_isSampling || _isCalibrating) {
        Serial.println("[EMG_MANAGER] Cannot calibrate while sampling or already calibrating");
        return;
    }

    Serial.println("[EMG_MANAGER] Starting calibration...");
    _isCalibrating = true;
    _calibrationPhase = 0;
    _calibrationStartTime = millis();
    _calibrationSamples.clear();
    _calibrated = false;

    // Start sampling task for calibration
    xTaskCreatePinnedToCore(
        this->samplingTask,
        "EMGCalibrationTask",
        4096,
        this,
        10,
        &_samplingTaskHandle,
        1
    );

    Serial.println("[EMG_MANAGER] Phase 0: RESTING - Relax muscle completely (10 seconds)");
}

void EMGSensorManager::cancelCalibration() {
    if (!_isCalibrating) return;

    _isCalibrating = false;
    _calibrationPhase = 0;
    _calibrationSamples.clear();

    if (_samplingTaskHandle != NULL) {
        vTaskDelete(_samplingTaskHandle);
        _samplingTaskHandle = NULL;
    }

    Serial.println("[EMG_MANAGER] Calibration cancelled");
}

void EMGSensorManager::processCalibrationData() {
    if (_calibrationSamples.empty()) return;

    unsigned long elapsed = millis() - _calibrationStartTime;

    // Phase 0: Baseline (reposo) - 10 segundos
    if (_calibrationPhase == 0 && elapsed >= CALIB_PHASE0_DURATION_MS) {
        // Calculate baseline
        long sum = 0;
        for (int s : _calibrationSamples) sum += s;
        _baseline = (float)sum / _calibrationSamples.size();

        Serial.printf("[EMG_MANAGER] Baseline captured: %.2f\n", _baseline);
        Serial.println("[EMG_MANAGER] Phase 1: MAXIMUM CONTRACTION - Contract muscle at maximum (5 seconds)");

        // Move to next phase
        _calibrationPhase = 1;
        _calibrationStartTime = millis();
        _calibrationSamples.clear();
    }
    // Phase 1: Maximum value (contracción máxima) - 5 segundos
    else if (_calibrationPhase == 1 && elapsed >= CALIB_PHASE1_DURATION_MS) {
        // Calculate max value
        _maxValue = 0;
        for (int s : _calibrationSamples) {
            if (s > _maxValue) _maxValue = s;
        }

        Serial.printf("[EMG_MANAGER] Maximum value captured: %.2f\n", _maxValue);

        // Validate calibration
        if (_maxValue > _baseline + CALIB_MIN_RANGE) {
            _calibrated = true;
            Serial.println("[EMG_MANAGER] ✅ Calibration SUCCESSFUL");
            Serial.printf("[EMG_MANAGER] Range: %.2f - %.2f\n", _baseline, _maxValue);
        } else {
            Serial.println("[EMG_MANAGER] ❌ Calibration FAILED - Insufficient range");
            _calibrated = false;
        }

        // Stop calibration
        _isCalibrating = false;
        if (_samplingTaskHandle != NULL) {
            vTaskDelete(_samplingTaskHandle);
            _samplingTaskHandle = NULL;
        }
        _calibrationSamples.clear();
    }
}

float EMGSensorManager::getCurrentValue() const {
    int raw = analogRead(_pin);
    if (!_calibrated) return (float)raw;
    float normalized = (raw - _baseline) / (_maxValue - _baseline);
    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;
    return normalized;
}

float EMGSensorManager::normalizeValue(int rawValue) {
    if (!_calibrated) return rawValue;

    // Normalize to 0.0 - 1.0 range
    float normalized = (rawValue - _baseline) / (_maxValue - _baseline);

    // Clamp to 0.0 - 1.0
    if (normalized < 0.0) normalized = 0.0;
    if (normalized > 1.0) normalized = 1.0;

    return normalized;
}

std::string EMGSensorManager::getCalibrationStatus() {
    std::string json = "{";
    json += "\"calibrated\":" + std::string(_calibrated ? "true" : "false") + ",";
    json += "\"calibrating\":" + std::string(_isCalibrating ? "true" : "false") + ",";
    json += "\"phase\":" + std::to_string(_calibrationPhase) + ",";
    json += "\"baseline\":" + std::to_string(_baseline) + ",";
    json += "\"maxValue\":" + std::to_string(_maxValue);

    if (_isCalibrating) {
        unsigned long elapsed = millis() - _calibrationStartTime;
        unsigned long remaining = 0;
        std::string phaseDesc = "";

        if (_calibrationPhase == 0) {
            remaining = 10000 - elapsed;
            phaseDesc = "RESTING - Relax muscle completely";
        } else if (_calibrationPhase == 1) {
            remaining = 5000 - elapsed;
            phaseDesc = "MAXIMUM - Contract muscle at maximum";
        }

        json += ",\"remainingMs\":" + std::to_string(remaining);
        json += ",\"phaseDescription\":\"" + phaseDesc + "\"";
    }

    json += "}";
    return json;
}
