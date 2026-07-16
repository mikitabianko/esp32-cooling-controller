#pragma once

#include "core/Clock.h"

class Esp32Clock : public Clock {
public:
  uint32_t nowMs() const override;
};
