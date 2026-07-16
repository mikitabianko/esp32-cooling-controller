#pragma once

#include "cooling_feature/CoolingFeature.h"
#include "app/AppContext.h"
#include "fw_esp32/Esp32Clock.h"
#include "fw_esp32/Esp32SystemControl.h"
#include "fw_esp32/PreferencesSettingsBackend.h"
#include "network/NetworkManager.h"
#include "ota/FirmwareUpdateService.h"
#include "ota/OtaManager.h"
#include "telemetry/TelemetryRegistry.h"
#include "web/WebHost.h"

class CoolingFirmware {
public:
  CoolingFirmware();

  void begin();
  void update();

private:
  Esp32Clock clock_;
  PreferencesSettingsBackend settings_;
  Esp32SystemControl system_;
  NetworkManager network_;
  WebHost web_;
  OtaManager ota_;
  FirmwareUpdateService firmwareUpdate_;
  TelemetryRegistry telemetry_;
  AppContext context_;
  CoolingFeature cooling_;
};
