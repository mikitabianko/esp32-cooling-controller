#include <unity.h>
#include "domain/TemperatureLogic.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace {

constexpr float kDisconnectedMarkerC = -127.0F;

void test_classifies_disconnected_temperature()
{
  TEST_ASSERT_EQUAL(TemperatureStatus::Disconnected,
                    classifyTemperature(kDisconnectedMarkerC,
                                        kDisconnectedMarkerC));
}

void test_classifies_regular_temperature()
{
  TEST_ASSERT_EQUAL(TemperatureStatus::Ok,
                    classifyTemperature(23.5F, kDisconnectedMarkerC));
}

void test_exact_marker_is_required_for_disconnected_state()
{
  TEST_ASSERT_EQUAL(TemperatureStatus::Ok,
                    classifyTemperature(-126.9F, kDisconnectedMarkerC));
}

void test_update_count_increments_by_one()
{
  TEST_ASSERT_EQUAL(1, nextUpdateCount(0));
  TEST_ASSERT_EQUAL(42, nextUpdateCount(41));
}

void test_cooling_starts_above_hysteresis_band()
{
  TEST_ASSERT_TRUE(shouldCoolingRun(6.1F, 5.0F, 1.0F, false));
}

void test_cooling_stays_off_inside_hysteresis_band()
{
  TEST_ASSERT_FALSE(shouldCoolingRun(5.8F, 5.0F, 1.0F, false));
}

void test_cooling_keeps_running_until_target_is_reached()
{
  TEST_ASSERT_TRUE(shouldCoolingRun(5.1F, 5.0F, 1.0F, true));
  TEST_ASSERT_FALSE(shouldCoolingRun(5.0F, 5.0F, 1.0F, true));
}

} // namespace

void runTemperatureLogicTests()
{
  UNITY_BEGIN();
  RUN_TEST(test_classifies_disconnected_temperature);
  RUN_TEST(test_classifies_regular_temperature);
  RUN_TEST(test_exact_marker_is_required_for_disconnected_state);
  RUN_TEST(test_update_count_increments_by_one);
  RUN_TEST(test_cooling_starts_above_hysteresis_band);
  RUN_TEST(test_cooling_stays_off_inside_hysteresis_band);
  RUN_TEST(test_cooling_keeps_running_until_target_is_reached);
  UNITY_END();
}

#ifdef ARDUINO
void setup()
{
  delay(2000);
  runTemperatureLogicTests();
}

void loop()
{
}
#else
int main()
{
  runTemperatureLogicTests();
  return 0;
}
#endif
