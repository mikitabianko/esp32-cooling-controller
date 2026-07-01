#pragma once

#include "domain/AppSettings.h"

struct CoolingState {
  bool peltierRunning = false;
  bool fanRunning = false;
  bool fanRunOnActive = false;
  unsigned long fanRunOnRemainingMs = 0;
};

class CoolingController {
public:
  void setSettings(const AppSettings &settings);
  const CoolingState &update(float temperatureC, unsigned long nowMs);
  bool running() const;
  const CoolingState &state() const;

private:
  void startFanRunOn(unsigned long nowMs);
  void updateFanRunOn(unsigned long nowMs);
  void logState() const;

  CoolingState state_;
  AppSettings settings_;
  unsigned long fanRunOnStartedMs_ = 0;
};
