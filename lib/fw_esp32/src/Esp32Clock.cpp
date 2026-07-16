#include "fw_esp32/Esp32Clock.h"

#include <Arduino.h>

uint32_t Esp32Clock::nowMs() const
{
  return millis();
}
