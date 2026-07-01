#pragma once

#include <DallasTemperature.h>
#include <OneWire.h>
#include "domain/AppSettings.h"

class TemperatureProbe {
public:
  TemperatureProbe();

  void begin();
  void setSettings(const AppSettings &settings);
  bool update(unsigned long nowMs, float &temperatureC);
  void recordUpdate();
  int updateCount() const;

private:
  void requestTemperature(unsigned long nowMs);
  bool conversionReady(unsigned long nowMs) const;

  OneWire oneWire_;
  DallasTemperature sensors_;
  int updateCount_ = 0;
  AppSettings settings_;
  bool conversionPending_ = false;
  bool requestSent_ = false;
  unsigned long requestedAtMs_ = 0;
};
