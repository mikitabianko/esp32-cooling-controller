#pragma once

#include <Arduino.h>

class RelayModule {
public:
  void begin();
  void startSelfTest(unsigned long nowMs);
  void updateSelfTest(unsigned long nowMs);
  bool selfTestRunning() const;
  void setPeltier(bool enabled);
  void setFan(bool enabled);
  void setCooling(bool enabled);
  void setOutputs(bool peltierEnabled, bool fanEnabled);

private:
  enum class SelfTestState {
    Idle,
    CoolingOn,
    CoolingOff
  };

  SelfTestState selfTestState_ = SelfTestState::Idle;
  unsigned long selfTestStateStartedMs_ = 0;
};
