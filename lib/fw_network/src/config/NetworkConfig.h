#pragma once

#include <Arduino.h>

namespace NetworkConfigLimits {
constexpr size_t MaxStationSsidLength = 32U;
constexpr size_t MaxStationPasswordLength = 64U;
} // namespace NetworkConfigLimits

struct NetworkConfig {
  String stationSsid;
  String stationPassword;
};

NetworkConfig sanitizeNetworkConfig(const NetworkConfig &config);
