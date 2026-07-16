#pragma once

#include "core/SettingsBackend.h"
#include "config/DeviceConfig.h"

class SettingsStore {
public:
  void begin(SettingsBackend &backend);
  DeviceConfig load();
  bool save(const DeviceConfig &config);

private:
  SettingsBackend *backend_ = nullptr;
};
