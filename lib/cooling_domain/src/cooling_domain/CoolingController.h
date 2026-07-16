#pragma once

#include <stdint.h>

#include "cooling_domain/CoolingConfig.h"

struct CoolingInputs {
  float temperatureC = 0.0F;
  bool sensorAvailable = false;
};

struct CoolingOutputs {
  bool peltierEnabled = false;
  bool fanEnabled = false;
  bool fanRunOnActive = false;
  uint32_t fanRunOnRemainingMs = 0U;
};

class CoolingController {
public:
  CoolingOutputs step(const CoolingInputs &inputs,
                      const CoolingConfig &config,
                      uint32_t nowMs);
  const CoolingOutputs &outputs() const;

private:
  void startFanRunOn(uint32_t nowMs);
  void updateFanRunOn(uint32_t nowMs, uint32_t fanRunOnMs);

  CoolingOutputs outputs_;
  uint32_t fanRunOnStartedMs_ = 0U;
};
