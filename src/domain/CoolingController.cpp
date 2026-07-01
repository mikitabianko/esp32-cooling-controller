#include "domain/CoolingController.h"

#include <Arduino.h>
#include "domain/TemperatureLogic.h"

namespace {

bool isSensorDisconnected(float temperatureC)
{
  return classifyTemperature(temperatureC) == TemperatureStatus::Disconnected;
}

} // namespace

void CoolingController::setSettings(const AppSettings &settings)
{
  settings_ = sanitizedSettings(settings);
}

const CoolingState &CoolingController::update(float temperatureC,
                                              unsigned long nowMs)
{
  const bool wasPeltierRunning = state_.peltierRunning;

  if (isSensorDisconnected(temperatureC)) {
    state_.peltierRunning = false;
  } else {
    state_.peltierRunning = shouldCoolingRun(
        temperatureC,
        settings_.targetTemperatureC,
        settings_.hysteresisC,
        state_.peltierRunning);
  }

  if (wasPeltierRunning && !state_.peltierRunning) {
    startFanRunOn(nowMs);
  }

  updateFanRunOn(nowMs);
  state_.fanRunning = state_.peltierRunning || state_.fanRunOnActive;

  logState();
  return state_;
}

bool CoolingController::running() const
{
  return state_.peltierRunning;
}

const CoolingState &CoolingController::state() const
{
  return state_;
}

void CoolingController::startFanRunOn(unsigned long nowMs)
{
  state_.fanRunOnActive = true;
  fanRunOnStartedMs_ = nowMs;
}

void CoolingController::updateFanRunOn(unsigned long nowMs)
{
  if (!state_.fanRunOnActive) {
    state_.fanRunOnRemainingMs = 0;
    return;
  }

  const unsigned long elapsedMs = nowMs - fanRunOnStartedMs_;
  if (elapsedMs >= settings_.fanRunOnMs || state_.peltierRunning) {
    state_.fanRunOnActive = false;
    state_.fanRunOnRemainingMs = 0;
    return;
  }

  state_.fanRunOnRemainingMs = settings_.fanRunOnMs - elapsedMs;
}

void CoolingController::logState() const
{
  Serial.print("Peltier: ");
  Serial.print(state_.peltierRunning ? "ON" : "OFF");
  Serial.print(" | fan: ");
  Serial.print(state_.fanRunning ? "ON" : "OFF");
  if (state_.fanRunOnActive) {
    Serial.print(" | run-on ");
    Serial.print(state_.fanRunOnRemainingMs / 1000);
    Serial.print(" s");
  }
  Serial.print(" | target ");
  Serial.print(settings_.targetTemperatureC, 1);
  Serial.print(" C | hysteresis ");
  Serial.print(settings_.hysteresisC, 1);
  Serial.println(" C");
}
