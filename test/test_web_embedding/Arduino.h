#pragma once

#include <string>

#define PROGMEM

class String {
public:
  String(const char *value)
      : value_(value == nullptr ? "" : value)
  {
  }

  bool operator==(const char *other) const
  {
    return value_ == (other == nullptr ? "" : other);
  }

private:
  std::string value_;
};
