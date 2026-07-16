#include "cooling_domain/CoolingController.h"

CoolingOutputs CoolingController::step(const CoolingInputs &inputs,
                                       const CoolingConfig &config,
                                       uint32_t nowMs)
{
  const CoolingConfig sanitized = sanitizeCoolingConfig(config);
  const bool wasPeltierEnabled = outputs_.peltierEnabled;

  if (!inputs.sensorAvailable) {
    outputs_.peltierEnabled = false;
  } else if (outputs_.peltierEnabled) {
    outputs_.peltierEnabled =
        inputs.temperatureC > sanitized.targetTemperatureC;
  } else {
    outputs_.peltierEnabled =
        inputs.temperatureC >
        sanitized.targetTemperatureC + sanitized.hysteresisC;
  }

  if (wasPeltierEnabled && !outputs_.peltierEnabled) {
    startFanRunOn(nowMs);
  }

  updateFanRunOn(nowMs, sanitized.fanRunOnMs);
  outputs_.fanEnabled = outputs_.peltierEnabled || outputs_.fanRunOnActive;
  return outputs_;
}

const CoolingOutputs &CoolingController::outputs() const
{
  return outputs_;
}

void CoolingController::startFanRunOn(uint32_t nowMs)
{
  outputs_.fanRunOnActive = true;
  fanRunOnStartedMs_ = nowMs;
}

void CoolingController::updateFanRunOn(uint32_t nowMs, uint32_t fanRunOnMs)
{
  if (!outputs_.fanRunOnActive) {
    outputs_.fanRunOnRemainingMs = 0U;
    return;
  }

  const uint32_t elapsedMs = nowMs - fanRunOnStartedMs_;
  if (elapsedMs >= fanRunOnMs || outputs_.peltierEnabled) {
    outputs_.fanRunOnActive = false;
    outputs_.fanRunOnRemainingMs = 0U;
    return;
  }

  outputs_.fanRunOnRemainingMs = fanRunOnMs - elapsedMs;
}
