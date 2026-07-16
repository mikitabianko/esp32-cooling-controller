#pragma once

#include "core/SystemControl.h"

class Esp32SystemControl : public SystemControl {
public:
  void restart() override;
};
