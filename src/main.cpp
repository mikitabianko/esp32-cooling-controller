#include <Arduino.h>
#include "product/CoolingFirmware.h"

CoolingFirmware firmware;

void setup()
{
  firmware.begin();
}

void loop()
{
  firmware.update();
}
