#include <cmath>
#include <stdint.h>
#include <unity.h>

#include "cooling_domain/CoolingController.h"

namespace {

CoolingInputs availableAt(float temperatureC)
{
  CoolingInputs inputs;
  inputs.temperatureC = temperatureC;
  inputs.sensorAvailable = true;
  return inputs;
}

void test_cooling_uses_hysteresis_to_start_and_target_to_stop()
{
  CoolingController controller;
  CoolingConfig config;

  TEST_ASSERT_FALSE(controller.step(availableAt(5.1F), config, 0U)
                        .peltierEnabled);
  TEST_ASSERT_TRUE(controller.step(availableAt(5.2F), config, 1U)
                       .peltierEnabled);
  TEST_ASSERT_TRUE(controller.step(availableAt(5.01F), config, 2U)
                       .peltierEnabled);
  TEST_ASSERT_FALSE(controller.step(availableAt(5.0F), config, 3U)
                        .peltierEnabled);
}

void test_fan_runs_on_after_peltier_stops()
{
  CoolingController controller;
  CoolingConfig config;
  config.fanRunOnMs = 1000U;

  controller.step(availableAt(8.0F), config, 100U);
  CoolingOutputs outputs = controller.step(availableAt(5.0F), config, 200U);
  TEST_ASSERT_FALSE(outputs.peltierEnabled);
  TEST_ASSERT_TRUE(outputs.fanEnabled);
  TEST_ASSERT_TRUE(outputs.fanRunOnActive);
  TEST_ASSERT_EQUAL_UINT32(1000U, outputs.fanRunOnRemainingMs);

  outputs = controller.step(availableAt(5.0F), config, 700U);
  TEST_ASSERT_EQUAL_UINT32(500U, outputs.fanRunOnRemainingMs);

  outputs = controller.step(availableAt(5.0F), config, 1200U);
  TEST_ASSERT_FALSE(outputs.fanEnabled);
  TEST_ASSERT_FALSE(outputs.fanRunOnActive);
  TEST_ASSERT_EQUAL_UINT32(0U, outputs.fanRunOnRemainingMs);
}

void test_resumed_cooling_cancels_fan_run_on()
{
  CoolingController controller;
  CoolingConfig config;

  controller.step(availableAt(8.0F), config, 10U);
  TEST_ASSERT_TRUE(controller.step(availableAt(5.0F), config, 20U)
                       .fanRunOnActive);
  const CoolingOutputs outputs =
      controller.step(availableAt(8.0F), config, 30U);
  TEST_ASSERT_TRUE(outputs.peltierEnabled);
  TEST_ASSERT_TRUE(outputs.fanEnabled);
  TEST_ASSERT_FALSE(outputs.fanRunOnActive);
}

void test_disconnected_sensor_forces_peltier_off()
{
  CoolingController controller;
  CoolingConfig config;
  controller.step(availableAt(8.0F), config, 100U);

  CoolingInputs disconnected;
  disconnected.temperatureC = 100.0F;
  disconnected.sensorAvailable = false;
  const CoolingOutputs outputs = controller.step(disconnected, config, 200U);

  TEST_ASSERT_FALSE(outputs.peltierEnabled);
  TEST_ASSERT_TRUE(outputs.fanEnabled);
  TEST_ASSERT_TRUE(outputs.fanRunOnActive);
}

void test_disconnected_sensor_never_starts_outputs_from_idle()
{
  CoolingController controller;
  CoolingConfig config;
  CoolingInputs disconnected;
  disconnected.temperatureC = 100.0F;
  disconnected.sensorAvailable = false;

  const CoolingOutputs outputs = controller.step(disconnected, config, 0U);
  TEST_ASSERT_FALSE(outputs.peltierEnabled);
  TEST_ASSERT_FALSE(outputs.fanEnabled);
}

void test_fan_run_on_timing_is_millis_overflow_safe()
{
  CoolingController controller;
  CoolingConfig config;
  config.fanRunOnMs = 50U;

  controller.step(availableAt(8.0F), config, UINT32_MAX - 20U);
  controller.step(availableAt(5.0F), config, UINT32_MAX - 10U);
  CoolingOutputs outputs = controller.step(availableAt(5.0F), config, 14U);
  TEST_ASSERT_TRUE(outputs.fanRunOnActive);
  TEST_ASSERT_EQUAL_UINT32(25U, outputs.fanRunOnRemainingMs);

  outputs = controller.step(availableAt(5.0F), config, 39U);
  TEST_ASSERT_FALSE(outputs.fanRunOnActive);
  TEST_ASSERT_FALSE(outputs.fanEnabled);
}

void test_cooling_config_sanitizes_bounds_and_non_finite_values()
{
  CoolingConfig config;
  config.targetTemperatureC = NAN;
  config.hysteresisC = 99.0F;
  config.measurementIntervalMs = 1U;
  config.fanRunOnMs = 900000U;

  const CoolingConfig sanitized = sanitizeCoolingConfig(config);
  TEST_ASSERT_FLOAT_WITHIN(0.001F,
                           CoolingDefaults::TargetTemperatureC,
                           sanitized.targetTemperatureC);
  TEST_ASSERT_FLOAT_WITHIN(0.001F,
                           CoolingLimits::MaxHysteresisC,
                           sanitized.hysteresisC);
  TEST_ASSERT_EQUAL_UINT32(CoolingLimits::MinMeasurementIntervalMs,
                           sanitized.measurementIntervalMs);
  TEST_ASSERT_EQUAL_UINT32(CoolingLimits::MaxFanRunOnMs,
                           sanitized.fanRunOnMs);
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_cooling_uses_hysteresis_to_start_and_target_to_stop);
  RUN_TEST(test_fan_runs_on_after_peltier_stops);
  RUN_TEST(test_resumed_cooling_cancels_fan_run_on);
  RUN_TEST(test_disconnected_sensor_forces_peltier_off);
  RUN_TEST(test_disconnected_sensor_never_starts_outputs_from_idle);
  RUN_TEST(test_fan_run_on_timing_is_millis_overflow_safe);
  RUN_TEST(test_cooling_config_sanitizes_bounds_and_non_finite_values);
  return UNITY_END();
}
