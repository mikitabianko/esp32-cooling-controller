#include "domain/AppSettings.h"

namespace {

float clampFloat(float value, float minValue, float maxValue)
{
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

unsigned long clampUnsignedLong(unsigned long value,
                                unsigned long minValue,
                                unsigned long maxValue)
{
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

String sanitizedString(const String &value, size_t maxLength)
{
  String sanitized = value;
  sanitized.trim();
  if (sanitized.length() > maxLength) {
    sanitized.remove(maxLength);
  }
  return sanitized;
}

} // namespace

AppSettings sanitizedSettings(const AppSettings &settings)
{
  AppSettings sanitized;
  sanitized.targetTemperatureC = clampFloat(
      settings.targetTemperatureC,
      Config::Control::MinTargetTemperatureC,
      Config::Control::MaxTargetTemperatureC);
  sanitized.hysteresisC = clampFloat(settings.hysteresisC,
                                     Config::Control::MinHysteresisC,
                                     Config::Control::MaxHysteresisC);
  sanitized.measurementIntervalMs = clampUnsignedLong(
      settings.measurementIntervalMs,
      Config::Control::MinMeasurementIntervalMs,
      Config::Control::MaxMeasurementIntervalMs);
  sanitized.fanRunOnMs = clampUnsignedLong(settings.fanRunOnMs,
                                           Config::Control::MinFanRunOnMs,
                                           Config::Control::MaxFanRunOnMs);
  sanitized.stationSsid =
      sanitizedString(settings.stationSsid, Config::Network::MaxStationSsidLength);
  sanitized.stationPassword = sanitizedString(
      settings.stationPassword,
      Config::Network::MaxStationPasswordLength);
  return sanitized;
}
