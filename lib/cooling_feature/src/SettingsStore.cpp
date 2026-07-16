#include "storage/SettingsStore.h"

namespace {

constexpr const char *kNamespace = "cooling";
constexpr const char *kTargetKey = "targetC";
constexpr const char *kHysteresisKey = "hysC";
constexpr const char *kMeasureKey = "measureMs";
constexpr const char *kFanRunOnKey = "fanRunMs";
constexpr const char *kStationSsidKey = "staSsid";
constexpr const char *kStationPasswordKey = "staPass";

} // namespace

void SettingsStore::begin(SettingsBackend &backend)
{
  backend_ = &backend;
  backend_->begin(kNamespace);
}

DeviceConfig SettingsStore::load()
{
  DeviceConfig config;
  char stationSsid[NetworkConfigLimits::MaxStationSsidLength + 1U] = {};
  char stationPassword[NetworkConfigLimits::MaxStationPasswordLength + 1U] = {};
  config.cooling.targetTemperatureC = backend_->getFloat(
      kTargetKey, config.cooling.targetTemperatureC);
  config.cooling.hysteresisC = backend_->getFloat(
      kHysteresisKey, config.cooling.hysteresisC);
  config.cooling.measurementIntervalMs = backend_->getUInt32(
      kMeasureKey, config.cooling.measurementIntervalMs);
  config.cooling.fanRunOnMs = backend_->getUInt32(
      kFanRunOnKey, config.cooling.fanRunOnMs);
  backend_->getString(kStationSsidKey,
                      config.network.stationSsid.c_str(),
                      stationSsid,
                      sizeof(stationSsid));
  backend_->getString(kStationPasswordKey,
                      config.network.stationPassword.c_str(),
                      stationPassword,
                      sizeof(stationPassword));
  config.network.stationSsid = stationSsid;
  config.network.stationPassword = stationPassword;
  return sanitizeDeviceConfig(config);
}

bool SettingsStore::save(const DeviceConfig &config)
{
  const DeviceConfig sanitized = sanitizeDeviceConfig(config);
  bool ok = true;
  ok = backend_->putFloat(kTargetKey,
                          sanitized.cooling.targetTemperatureC) && ok;
  ok = backend_->putFloat(kHysteresisKey,
                          sanitized.cooling.hysteresisC) && ok;
  ok = backend_->putUInt32(kMeasureKey,
                           sanitized.cooling.measurementIntervalMs) && ok;
  ok = backend_->putUInt32(kFanRunOnKey,
                           sanitized.cooling.fanRunOnMs) && ok;
  ok = backend_->putString(kStationSsidKey,
                           sanitized.network.stationSsid.c_str()) && ok;
  ok = backend_->putString(kStationPasswordKey,
                           sanitized.network.stationPassword.c_str()) && ok;
  return ok;
}
