#include <cstring>
#include <unity.h>

#include "../../lib/fw_core/src/core/Clock.h"
#include "../../lib/fw_core/src/core/SettingsBackend.h"
#include "../../lib/fw_telemetry/src/telemetry/TelemetryRegistry.h"

namespace {

class FakeClock : public Clock {
public:
  uint32_t nowMs() const override { return nowMs_; }
  uint32_t nowMs_ = 0U;
};

class FakeSettingsBackend : public SettingsBackend {
public:
  void begin(const char *nameSpace) override { nameSpace_ = nameSpace; }
  float getFloat(const char *, float fallback) const override { return fallback; }
  uint32_t getUInt32(const char *, uint32_t fallback) const override
  {
    return fallback;
  }
  void getString(const char *,
                 const char *fallback,
                 char *output,
                 size_t outputSize) const override
  {
    std::strncpy(output, fallback, outputSize - 1U);
    output[outputSize - 1U] = '\0';
  }
  bool putFloat(const char *, float) override { return true; }
  bool putUInt32(const char *, uint32_t) override { return true; }
  bool putString(const char *, const char *) override { return true; }

  const char *nameSpace_ = nullptr;
};

void test_clock_is_replaceable_through_interface()
{
  FakeClock clock;
  clock.nowMs_ = 1234U;
  Clock &service = clock;
  TEST_ASSERT_EQUAL_UINT32(1234U, service.nowMs());
}

void test_settings_backend_is_replaceable_through_interface()
{
  FakeSettingsBackend backend;
  SettingsBackend &service = backend;
  service.begin("test");
  char value[16] = {};
  service.getString("missing", "fallback", value, sizeof(value));
  TEST_ASSERT_EQUAL_STRING("test", backend.nameSpace_);
  TEST_ASSERT_EQUAL_STRING("fallback", value);
}

void test_telemetry_registry_tracks_feature_sources()
{
  TelemetryRegistry registry;
  TEST_ASSERT_TRUE(registry.registerSource("cooling"));
  TEST_ASSERT_EQUAL_UINT32(1U, registry.sourceCount());
  TEST_ASSERT_EQUAL_STRING("cooling", registry.sourceAt(0));
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_clock_is_replaceable_through_interface);
  RUN_TEST(test_settings_backend_is_replaceable_through_interface);
  RUN_TEST(test_telemetry_registry_tracks_feature_sources);
  return UNITY_END();
}
