#pragma once

#include <stddef.h>
#include <stdint.h>

class SettingsBackend {
public:
  virtual ~SettingsBackend() = default;
  virtual void begin(const char *nameSpace) = 0;
  virtual float getFloat(const char *key, float fallback) const = 0;
  virtual uint32_t getUInt32(const char *key, uint32_t fallback) const = 0;
  virtual void getString(const char *key,
                         const char *fallback,
                         char *output,
                         size_t outputSize) const = 0;
  virtual bool putFloat(const char *key, float value) = 0;
  virtual bool putUInt32(const char *key, uint32_t value) = 0;
  virtual bool putString(const char *key, const char *value) = 0;
};
