#pragma once

#include <stdint.h>

namespace CoolingDefaults {
constexpr float TargetTemperatureC = 5.0F;
constexpr float HysteresisC = 0.1F;
constexpr uint32_t MeasurementIntervalMs = 500U;
constexpr uint32_t FanRunOnMs = 30000U;
} // namespace CoolingDefaults

namespace CoolingLimits {
constexpr float MinTargetTemperatureC = -20.0F;
constexpr float MaxTargetTemperatureC = 40.0F;
constexpr float MinHysteresisC = 0.1F;
constexpr float MaxHysteresisC = 10.0F;
constexpr uint32_t MinMeasurementIntervalMs = 250U;
constexpr uint32_t MaxMeasurementIntervalMs = 60000U;
constexpr uint32_t MinFanRunOnMs = 0U;
constexpr uint32_t MaxFanRunOnMs = 600000U;
} // namespace CoolingLimits

struct CoolingConfig {
  float targetTemperatureC = CoolingDefaults::TargetTemperatureC;
  float hysteresisC = CoolingDefaults::HysteresisC;
  uint32_t measurementIntervalMs = CoolingDefaults::MeasurementIntervalMs;
  uint32_t fanRunOnMs = CoolingDefaults::FanRunOnMs;
};

CoolingConfig sanitizeCoolingConfig(const CoolingConfig &config);
