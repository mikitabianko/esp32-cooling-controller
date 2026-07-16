#pragma once

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <string>

#define HIGH 1
#define LOW 0

class String {
public:
  String() = default;
  String(const char *value)
      : value_(value == nullptr ? "" : value)
  {
  }

  size_t length() const { return value_.length(); }
  const char *c_str() const { return value_.c_str(); }

  void trim()
  {
    size_t first = 0;
    while (first < value_.size() &&
           isspace(static_cast<unsigned char>(value_[first]))) {
      ++first;
    }
    size_t last = value_.size();
    while (last > first &&
           isspace(static_cast<unsigned char>(value_[last - 1]))) {
      --last;
    }
    value_ = value_.substr(first, last - first);
  }

  void remove(size_t index)
  {
    if (index < value_.size()) {
      value_.erase(index);
    }
  }

  bool operator==(const char *other) const
  {
    return value_ == (other == nullptr ? "" : other);
  }

private:
  std::string value_;
};
