#include "config/DeviceConfig.h"

DeviceConfig sanitizeDeviceConfig(const DeviceConfig &config)
{
  DeviceConfig sanitized;
  sanitized.cooling = sanitizeCoolingConfig(config.cooling);
  sanitized.network = sanitizeNetworkConfig(config.network);
  return sanitized;
}
