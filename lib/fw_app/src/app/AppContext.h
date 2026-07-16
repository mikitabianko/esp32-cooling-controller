#pragma once

#include "core/Clock.h"
#include "core/SettingsBackend.h"
#include "network/NetworkManager.h"
#include "telemetry/TelemetryRegistry.h"

struct AppContext {
  Clock &clock;
  SettingsBackend &settings;
  NetworkManager &network;
  TelemetryRegistry &telemetry;
};
