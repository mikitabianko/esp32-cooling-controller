#include <Arduino.h>

#include "StatusFirmware.h"

StatusFirmware firmware;

void setup()
{
  firmware.begin();
}

void loop()
{
  firmware.update();
}
