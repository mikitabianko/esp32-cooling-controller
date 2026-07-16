#pragma once

#include <Arduino.h>

namespace JsonHelpers {

String escapedString(const String &value);
const char *boolText(bool value);

} // namespace JsonHelpers
