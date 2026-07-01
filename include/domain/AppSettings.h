#pragma once

#include <Arduino.h>
#include "config/Config.h"

struct AppSettings {
  float targetTemperatureC = Config::Control::DefaultTargetTemperatureC;
  float hysteresisC = Config::Control::DefaultHysteresisC;
  unsigned long measurementIntervalMs =
      Config::Control::DefaultMeasurementIntervalMs;
  unsigned long fanRunOnMs = Config::Control::DefaultFanRunOnMs;
  String stationSsid = Config::Network::StationSsid;
  String stationPassword = Config::Network::StationPassword;
};

AppSettings sanitizedSettings(const AppSettings &settings);
