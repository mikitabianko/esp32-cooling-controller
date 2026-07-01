#include "hardware/RelayModule.h"

#include <Arduino.h>
#include "config/Config.h"

void RelayModule::begin()
{
  pinMode(Config::Pins::PeltierRelay, OUTPUT);
  pinMode(Config::Pins::FanRelay, OUTPUT);
  setCooling(false);
}

void RelayModule::startSelfTest(unsigned long nowMs)
{
  Serial.println("Relay self-test: ON");
  setCooling(true);
  selfTestState_ = SelfTestState::CoolingOn;
  selfTestStateStartedMs_ = nowMs;
}

void RelayModule::updateSelfTest(unsigned long nowMs)
{
  if (selfTestState_ == SelfTestState::Idle) {
    return;
  }

  if (nowMs - selfTestStateStartedMs_ < Config::Relay::SelfTestPulseMs) {
    return;
  }

  if (selfTestState_ == SelfTestState::CoolingOn) {
    Serial.println("Relay self-test: OFF");
    setCooling(false);
    selfTestState_ = SelfTestState::CoolingOff;
    selfTestStateStartedMs_ = nowMs;
    return;
  }

  Serial.println("Relay self-test: DONE");
  selfTestState_ = SelfTestState::Idle;
}

bool RelayModule::selfTestRunning() const
{
  return selfTestState_ != SelfTestState::Idle;
}

void RelayModule::setCooling(bool enabled)
{
  setOutputs(enabled, enabled);
}

void RelayModule::setPeltier(bool enabled)
{
  digitalWrite(Config::Pins::PeltierRelay,
               enabled ? Config::Relay::OnLevel : Config::Relay::OffLevel);
}

void RelayModule::setFan(bool enabled)
{
  digitalWrite(Config::Pins::FanRelay,
               enabled ? Config::Relay::OnLevel : Config::Relay::OffLevel);
}

void RelayModule::setOutputs(bool peltierEnabled, bool fanEnabled)
{
  setPeltier(peltierEnabled);
  setFan(fanEnabled);
}
