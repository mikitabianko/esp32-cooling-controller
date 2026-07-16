#pragma once

#include <IPAddress.h>

#include "config/DeviceConfig.h"
#include "cooling_domain/CoolingController.h"
#include "app/AppContext.h"
#include "hardware/RelayModule.h"
#include "hardware/TemperatureProbe.h"
#include "ota/OtaManager.h"
#include "storage/SettingsStore.h"
#include "ui/OledView.h"
#include "web/CoolingApi.h"

class CoolingFeature {
public:
  void begin(AppContext &context);
  void tick(uint32_t nowMs);
  void registerApi(ApiRouter &router);
  void onNetworkStarted(const IPAddress &accessPointIp);
  void setOtaStatus(const OtaStatus &status);

private:
  static void showStartupProgress(void *feature,
                                  const char *message,
                                  unsigned long elapsedMs,
                                  unsigned long totalMs);
  void applyConfig(const DeviceConfig &config);
  void updateTemperature(uint32_t nowMs,
                         const TemperatureReading &reading);
  void logCoolingState(const CoolingOutputs &outputs) const;

  AppContext *context_ = nullptr;
  DeviceConfig config_;
  SettingsStore settingsStore_;
  RelayModule relays_;
  TemperatureProbe probe_;
  CoolingController controller_;
  OledView view_;
  CoolingApi api_;
};
