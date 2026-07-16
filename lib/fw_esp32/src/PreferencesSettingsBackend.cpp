#include "fw_esp32/PreferencesSettingsBackend.h"

#include <cstring>

void PreferencesSettingsBackend::begin(const char *nameSpace)
{
  preferences_.begin(nameSpace, false);
}

float PreferencesSettingsBackend::getFloat(const char *key,
                                            float fallback) const
{
  return preferences_.getFloat(key, fallback);
}

uint32_t PreferencesSettingsBackend::getUInt32(const char *key,
                                                uint32_t fallback) const
{
  return preferences_.getULong(key, fallback);
}

void PreferencesSettingsBackend::getString(const char *key,
                                           const char *fallback,
                                           char *output,
                                           size_t outputSize) const
{
  if (output == nullptr || outputSize == 0U) {
    return;
  }
  const String value = preferences_.getString(key, fallback);
  std::strncpy(output, value.c_str(), outputSize - 1U);
  output[outputSize - 1U] = '\0';
}

bool PreferencesSettingsBackend::putFloat(const char *key, float value)
{
  return preferences_.putFloat(key, value) > 0;
}

bool PreferencesSettingsBackend::putUInt32(const char *key, uint32_t value)
{
  return preferences_.putULong(key, value) > 0;
}

bool PreferencesSettingsBackend::putString(const char *key,
                                            const char *value)
{
  const char *safeValue = value == nullptr ? "" : value;
  return preferences_.putString(key, safeValue) >= std::strlen(safeValue);
}
