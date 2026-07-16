#pragma once

#include <Preferences.h>

#include "core/SettingsBackend.h"

class PreferencesSettingsBackend : public SettingsBackend {
public:
  void begin(const char *nameSpace) override;
  float getFloat(const char *key, float fallback) const override;
  uint32_t getUInt32(const char *key, uint32_t fallback) const override;
  void getString(const char *key,
                 const char *fallback,
                 char *output,
                 size_t outputSize) const override;
  bool putFloat(const char *key, float value) override;
  bool putUInt32(const char *key, uint32_t value) override;
  bool putString(const char *key, const char *value) override;

private:
  mutable Preferences preferences_;
};
