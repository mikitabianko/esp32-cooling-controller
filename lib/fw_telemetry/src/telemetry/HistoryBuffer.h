#pragma once

#include <stddef.h>

template <typename T, size_t Capacity>
class HistoryBuffer {
  static_assert(Capacity > 0, "HistoryBuffer capacity must be positive");

public:
  static constexpr size_t capacity() { return Capacity; }
  size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }

  const T &at(size_t index) const
  {
    return samples_[(start_ + index) % Capacity];
  }

  void push(const T &sample)
  {
    if (count_ < Capacity) {
      samples_[(start_ + count_) % Capacity] = sample;
      ++count_;
      return;
    }

    samples_[start_] = sample;
    start_ = (start_ + 1U) % Capacity;
  }

  void clear()
  {
    start_ = 0U;
    count_ = 0U;
  }

private:
  T samples_[Capacity] = {};
  size_t start_ = 0U;
  size_t count_ = 0U;
};
