#pragma once

#include "config/NetworkConfig.h"
#include "cooling_domain/CoolingConfig.h"

struct DeviceConfig {
  CoolingConfig cooling;
  NetworkConfig network;
};

DeviceConfig sanitizeDeviceConfig(const DeviceConfig &config);
