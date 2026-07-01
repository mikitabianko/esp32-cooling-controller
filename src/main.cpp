#include <Arduino.h>
#include "app/CoolingApp.h"

CoolingApp app;

void setup()
{
  app.begin();
}

void loop()
{
  app.update();
}
