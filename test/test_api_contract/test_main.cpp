#include <fstream>
#include <sstream>
#include <string>
#include <unity.h>

namespace {

std::string loadFixture(const char *name)
{
  const std::string sourcePath = __FILE__;
  const size_t testDirectory = sourcePath.rfind("/test_api_contract/");
  const std::string root = testDirectory == std::string::npos
                               ? "test"
                               : sourcePath.substr(0, testDirectory);
  std::ifstream input(root + "/fixtures/api/" + name);
  std::ostringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

void assertFields(const std::string &json,
                  const char *const *fields,
                  size_t fieldCount)
{
  TEST_ASSERT_FALSE(json.empty());
  for (size_t i = 0; i < fieldCount; ++i) {
    const std::string key = std::string("\"") + fields[i] + "\"";
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find(key));
  }
}

void test_status_response_contract_fields()
{
  const char *fields[] = {
      "temperatureC", "filteredTemperatureC", "hasTemperature",
      "sensorDisconnected", "updateCount", "peltierRunning", "fanRunning",
      "fanRunOnActive", "fanRunOnRemainingMs", "targetC", "hysteresisC",
      "measurementIntervalMs", "fanRunOnMs", "uptimeMs", "devMode",
      "accessPointSsid", "accessPointIp", "stationSsid", "stationConnected",
      "stationStatus", "stationLastFailure", "stationIp", "stationRssi",
      "otaEnabled", "otaStarted", "otaUpdating", "otaProgress", "otaStatus",
      "otaHostname", "otaPasswordSet", "otaLastError"};
  assertFields(loadFixture("status.json"), fields, sizeof(fields) / sizeof(fields[0]));
}

void test_settings_response_contract_fields()
{
  const char *fields[] = {
      "ok", "targetC", "hysteresisC", "measurementIntervalMs", "fanRunOnMs",
      "accessPointSsid", "accessPointIp", "stationSsid", "stationPasswordSet",
      "stationConnected", "stationStatus", "stationLastFailure", "stationIp",
      "stationRssi", "otaEnabled", "otaStarted", "otaUpdating", "otaProgress",
      "otaStatus", "otaHostname", "otaPasswordSet", "otaLastError"};
  assertFields(loadFixture("settings.json"), fields, sizeof(fields) / sizeof(fields[0]));
}

void test_history_response_contract_fields()
{
  const char *fields[] = {
      "ok", "capacity", "keepaliveMs", "changeThresholdCx10",
      "disconnectedFlag", "peltierFlag", "fanFlag", "fanRunOnFlag",
      "hysteresisC", "fields", "series", "id", "label", "unit", "points"};
  assertFields(loadFixture("history.json"), fields, sizeof(fields) / sizeof(fields[0]));
}

void test_blink_status_response_contract_fields()
{
  const char *fields[] = {
      "deviceName", "ledOn", "toggleCount", "blinkIntervalMs", "uptimeMs",
      "accessPointSsid", "accessPointIp", "stationSsid", "stationPasswordSet",
      "stationConnected", "stationStatus", "stationLastFailure", "stationIp",
      "stationRssi",
      "otaEnabled", "otaStarted", "otaUpdating", "otaProgress", "otaStatus",
      "otaHostname", "otaPasswordSet", "otaLastError"};
  assertFields(loadFixture("blink_status.json"),
               fields,
               sizeof(fields) / sizeof(fields[0]));
}

void test_blink_settings_response_contract_fields()
{
  const char *fields[] = {
      "ok", "deviceName", "blinkIntervalMs", "accessPointSsid",
      "accessPointIp", "stationSsid", "stationPasswordSet",
      "stationConnected", "stationStatus", "stationLastFailure", "stationIp",
      "stationRssi", "otaEnabled", "otaStarted", "otaUpdating",
      "otaProgress", "otaStatus", "otaHostname", "otaPasswordSet",
      "otaLastError"};
  assertFields(loadFixture("blink_settings.json"),
               fields,
               sizeof(fields) / sizeof(fields[0]));
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_status_response_contract_fields);
  RUN_TEST(test_settings_response_contract_fields);
  RUN_TEST(test_history_response_contract_fields);
  RUN_TEST(test_blink_status_response_contract_fields);
  RUN_TEST(test_blink_settings_response_contract_fields);
  return UNITY_END();
}
