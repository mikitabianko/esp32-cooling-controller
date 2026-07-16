#include <string>
#include <unity.h>

#include "../../lib/fw_network/src/config/NetworkConfig.h"
#include "../../lib/fw_network/src/NetworkConfig.cpp"

namespace {

void test_network_config_trims_credentials()
{
  NetworkConfig config;
  config.stationSsid = "  workshop wifi  ";
  config.stationPassword = "  secret pass  ";

  const NetworkConfig sanitized = sanitizeNetworkConfig(config);
  TEST_ASSERT_EQUAL_STRING("workshop wifi", sanitized.stationSsid.c_str());
  TEST_ASSERT_EQUAL_STRING("secret pass", sanitized.stationPassword.c_str());
}

void test_network_config_limits_credential_lengths()
{
  NetworkConfig config;
  config.stationSsid = std::string(40, 's').c_str();
  config.stationPassword = std::string(80, 'p').c_str();

  const NetworkConfig sanitized = sanitizeNetworkConfig(config);
  TEST_ASSERT_EQUAL_UINT32(32U, sanitized.stationSsid.length());
  TEST_ASSERT_EQUAL_UINT32(64U, sanitized.stationPassword.length());
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_network_config_trims_credentials);
  RUN_TEST(test_network_config_limits_credential_lengths);
  return UNITY_END();
}
