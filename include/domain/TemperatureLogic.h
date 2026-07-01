#pragma once

enum class TemperatureStatus {
  Ok,
  Disconnected,
};

constexpr float kDisconnectedTemperatureC = -127.0F;

constexpr TemperatureStatus classifyTemperature(float temperatureC,
                                                float disconnectedMarkerC =
                                                    kDisconnectedTemperatureC)
{
  return temperatureC == disconnectedMarkerC ? TemperatureStatus::Disconnected
                                             : TemperatureStatus::Ok;
}

constexpr int nextUpdateCount(int currentCount)
{
  return currentCount + 1;
}

inline bool shouldCoolingRun(float temperatureC,
                             float targetTemperatureC,
                             float hysteresisC,
                             bool coolingCurrentlyRunning)
{
  if (coolingCurrentlyRunning) {
    return temperatureC > targetTemperatureC;
  }

  return temperatureC > targetTemperatureC + hysteresisC;
}
