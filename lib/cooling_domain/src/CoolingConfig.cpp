#include "cooling_domain/CoolingConfig.h"

#include <cmath>

namespace {

float sanitizeFloat(float value,
                    float fallback,
                    float minValue,
                    float maxValue)
{
  if (!std::isfinite(value)) {
    return fallback;
  }
  if (value < minValue) {
    return minValue;
  }
  return value > maxValue ? maxValue : value;
}

uint32_t clampUint32(uint32_t value, uint32_t minValue, uint32_t maxValue)
{
  if (value < minValue) {
    return minValue;
  }
  return value > maxValue ? maxValue : value;
}

} // namespace

CoolingConfig sanitizeCoolingConfig(const CoolingConfig &config)
{
  CoolingConfig sanitized;
  sanitized.targetTemperatureC = sanitizeFloat(
      config.targetTemperatureC,
      CoolingDefaults::TargetTemperatureC,
      CoolingLimits::MinTargetTemperatureC,
      CoolingLimits::MaxTargetTemperatureC);
  sanitized.hysteresisC = sanitizeFloat(config.hysteresisC,
                                        CoolingDefaults::HysteresisC,
                                        CoolingLimits::MinHysteresisC,
                                        CoolingLimits::MaxHysteresisC);
  sanitized.measurementIntervalMs = clampUint32(
      config.measurementIntervalMs,
      CoolingLimits::MinMeasurementIntervalMs,
      CoolingLimits::MaxMeasurementIntervalMs);
  sanitized.fanRunOnMs = clampUint32(config.fanRunOnMs,
                                     CoolingLimits::MinFanRunOnMs,
                                     CoolingLimits::MaxFanRunOnMs);
  return sanitized;
}
