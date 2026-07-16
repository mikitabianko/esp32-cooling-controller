#pragma once

#include <stdint.h>

class Clock {
public:
  virtual ~Clock() = default;
  virtual uint32_t nowMs() const = 0;
};
