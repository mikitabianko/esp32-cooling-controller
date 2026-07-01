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

void SettingsStore::begin()
{
  preferences_.begin(kNamespace, false);
}

AppSettings SettingsStore::load()
{
  AppSettings settings;
  settings.targetTemperatureC =
      preferences_.getFloat(kTargetKey, settings.targetTemperatureC);
  settings.hysteresisC =
      preferences_.getFloat(kHysteresisKey, settings.hysteresisC);
  settings.measurementIntervalMs =
      preferences_.getULong(kMeasureKey, settings.measurementIntervalMs);
  settings.fanRunOnMs =
      preferences_.getULong(kFanRunOnKey, settings.fanRunOnMs);
  settings.stationSsid =
      preferences_.getString(kStationSsidKey, settings.stationSsid);
  settings.stationPassword =
      preferences_.getString(kStationPasswordKey, settings.stationPassword);
  return sanitizedSettings(settings);
}

bool SettingsStore::save(const AppSettings &settings)
{
  const AppSettings sanitized = sanitizedSettings(settings);
  bool ok = true;
  ok = preferences_.putFloat(kTargetKey, sanitized.targetTemperatureC) > 0 && ok;
  ok = preferences_.putFloat(kHysteresisKey, sanitized.hysteresisC) > 0 && ok;
  ok = preferences_.putULong(kMeasureKey, sanitized.measurementIntervalMs) > 0 &&
       ok;
  ok = preferences_.putULong(kFanRunOnKey, sanitized.fanRunOnMs) > 0 && ok;
  ok = preferences_.putString(kStationSsidKey, sanitized.stationSsid) >=
           sanitized.stationSsid.length() &&
       ok;
  ok = preferences_.putString(kStationPasswordKey, sanitized.stationPassword) >=
           sanitized.stationPassword.length() &&
       ok;
  return ok;
}
