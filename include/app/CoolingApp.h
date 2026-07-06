#pragma once

#include <Arduino.h>
#include "domain/AppSettings.h"
#include "domain/CoolingController.h"
#include "hardware/RelayModule.h"
#include "hardware/TemperatureProbe.h"
#include "ota/OtaManager.h"
#include "storage/SettingsStore.h"
#include "ui/OledView.h"
#include "web/WebDashboard.h"

class CoolingApp {
public:
  void begin();
  void update();

private:
  void applySettings(const AppSettings &settings);
  void updateTemperature(unsigned long nowMs, float temperatureC);

  AppSettings settings_;
  SettingsStore settingsStore_;
  RelayModule relays_;
  TemperatureProbe probe_;
  CoolingController controller_;
  OledView view_;
  WebDashboard dashboard_;
  OtaManager ota_;
};
