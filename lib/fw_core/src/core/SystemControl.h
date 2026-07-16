#pragma once

class SystemControl {
public:
  virtual ~SystemControl() = default;
  virtual void restart() = 0;
};
