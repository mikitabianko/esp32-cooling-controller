#pragma once

#include <Preferences.h>
#include "domain/AppSettings.h"

class SettingsStore {
public:
  void begin();
  AppSettings load();
  bool save(const AppSettings &settings);

private:
  Preferences preferences_;
};
