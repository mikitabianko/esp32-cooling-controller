#pragma once

#include <DallasTemperature.h>
#include <OneWire.h>
#include "hardware/TemperatureProbeConfig.h"

struct TemperatureReading {
  float temperatureC = 0.0F;
  bool sensorAvailable = false;
};

class TemperatureProbe {
public:
  TemperatureProbe();

  void begin();
  void setConfig(const TemperatureProbeConfig &config);
  bool update(uint32_t nowMs, TemperatureReading &reading);
  void recordUpdate();
  int updateCount() const;

private:
  void requestTemperature(uint32_t nowMs);
  bool conversionReady(uint32_t nowMs) const;

  OneWire oneWire_;
  DallasTemperature sensors_;
  int updateCount_ = 0;
  TemperatureProbeConfig config_;
  bool conversionPending_ = false;
  bool requestSent_ = false;
  uint32_t requestedAtMs_ = 0U;
};
