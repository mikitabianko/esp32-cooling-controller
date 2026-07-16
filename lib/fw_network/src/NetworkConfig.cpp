#include "config/NetworkConfig.h"

namespace {

String sanitizeString(const String &value, size_t maxLength)
{
  String sanitized = value;
  sanitized.trim();
  if (sanitized.length() > maxLength) {
    sanitized.remove(maxLength);
  }
  return sanitized;
}

} // namespace

NetworkConfig sanitizeNetworkConfig(const NetworkConfig &config)
{
  NetworkConfig sanitized;
  sanitized.stationSsid = sanitizeString(
      config.stationSsid,
      NetworkConfigLimits::MaxStationSsidLength);
  sanitized.stationPassword = sanitizeString(
      config.stationPassword,
      NetworkConfigLimits::MaxStationPasswordLength);
  return sanitized;
}
