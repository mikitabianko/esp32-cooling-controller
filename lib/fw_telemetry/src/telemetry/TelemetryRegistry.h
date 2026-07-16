#pragma once

#include <stddef.h>

class TelemetryRegistry {
public:
  static constexpr size_t MaxSources = 8U;

  bool registerSource(const char *id)
  {
    if (id == nullptr || sourceCount_ >= MaxSources) {
      return false;
    }
    sources_[sourceCount_++] = id;
    return true;
  }

  size_t sourceCount() const { return sourceCount_; }
  const char *sourceAt(size_t index) const { return sources_[index]; }

private:
  const char *sources_[MaxSources] = {};
  size_t sourceCount_ = 0U;
};
