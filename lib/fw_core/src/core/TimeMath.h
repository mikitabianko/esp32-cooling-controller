#pragma once

#include <stdint.h>

inline bool deadlineReached(uint32_t nowMs, uint32_t deadlineMs)
{
  return static_cast<int32_t>(nowMs - deadlineMs) >= 0;
}
