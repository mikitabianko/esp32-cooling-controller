#include "fw_esp32/Esp32SystemControl.h"

#include <Arduino.h>

void Esp32SystemControl::restart()
{
  ESP.restart();
}
