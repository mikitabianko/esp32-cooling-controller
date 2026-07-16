#include "web/JsonHelpers.h"

namespace JsonHelpers {

String escapedString(const String &value)
{
  String escaped;
  escaped.reserve(value.length() + 2U);
  escaped += "\"";
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '"' || c == '\\') {
      escaped += "\\";
    }
    escaped += c;
  }
  escaped += "\"";
  return escaped;
}

const char *boolText(bool value)
{
  return value ? "true" : "false";
}

} // namespace JsonHelpers
