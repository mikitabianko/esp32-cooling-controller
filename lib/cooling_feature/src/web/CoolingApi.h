#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "config/NetworkConfig.h"
#include "cooling_domain/CoolingController.h"
#include "network/NetworkManager.h"
#include "ota/OtaManager.h"
#include "telemetry/HistoryBuffer.h"
#include "web/ApiRouter.h"

struct DashboardSnapshot {
  float temperatureC = 0.0F;
  bool hasTemperature = false;
  bool sensorAvailable = false;
  int updateCount = 0;
  CoolingOutputs coolingState;
  CoolingConfig coolingConfig;
  uint32_t uptimeMs = 0U;
};

struct DevDashboardState {
  bool enabled = false;
  float temperatureC = 5.0F;
  bool hasTemperature = true;
  bool sensorDisconnected = false;
  int updateCount = 0;
  CoolingOutputs coolingState;
};

struct TemperatureHistorySample {
  uint32_t uptimeMs = 0U;
  int16_t temperatureCx10 = 0;
  int16_t targetCx10 = 0;
  uint8_t flags = 0U;
};

class CoolingApi {
public:
  void begin(NetworkManager &network);
  void registerApi(ApiRouter &router);
  void update();
  void setSnapshot(const DashboardSnapshot &snapshot);
  void setConfig(const CoolingConfig &cooling,
                 const NetworkConfig &network);
  void setOtaStatus(const OtaStatus &status);
  bool takePendingConfig(CoolingConfig &cooling, NetworkConfig &network);

private:
  static constexpr size_t kEsp32StaticDramLimitBytes = 124580;
  static constexpr size_t kMeasuredStaticRamWithoutHistoryBytes = 50432;
  static constexpr size_t kTemperatureHistoryRamBudgetBytes =
      ((kEsp32StaticDramLimitBytes - kMeasuredStaticRamWithoutHistoryBytes) *
       9U) /
      10U;
  static constexpr size_t kTemperatureHistoryCapacity =
      kTemperatureHistoryRamBudgetBytes / sizeof(TemperatureHistorySample);
  static constexpr uint32_t kTemperatureHistoryKeepaliveMs = 30000U;
  static constexpr int16_t kTemperatureHistoryChangeCx10 = 1;
  static constexpr uint8_t kTemperatureHistoryDisconnectedFlag = 1U;
  static constexpr uint8_t kTemperatureHistoryPeltierFlag = 2U;
  static constexpr uint8_t kTemperatureHistoryFanFlag = 4U;
  static constexpr uint8_t kTemperatureHistoryFanRunOnFlag = 8U;
  using TemperatureHistory =
      HistoryBuffer<TemperatureHistorySample, kTemperatureHistoryCapacity>;

  void handleStatus();
  void handleHistory();
  void handleHistoryCsv();
  void handleGetDevState();
  void handleSaveDevState();
  void handleGetSettings();
  void handleSaveSettings();
  void handleNetworks();
  String statusJson() const;
  String historyJson() const;
  String historyCsv() const;
  String settingsJson() const;
  String devJson() const;
  void appendOtaJson(String &json) const;
  void queueConfigSave(const CoolingConfig &cooling,
                       const NetworkConfig &network);
  bool readConfigArgs(CoolingConfig &cooling, NetworkConfig &network);
  bool readDevArgs(DevDashboardState &state);
  bool readBoolArg(const char *name, bool &value);
  bool readStringArg(const char *name, String &value);
  bool readIntArg(const char *name, int &value);
  bool readFloatArg(const char *name, float &value);
  bool readUnsignedLongArg(const char *name, unsigned long &value);
  void recordTargetChange(uint32_t uptimeMs);
  void recordTemperatureHistory(const DashboardSnapshot &snapshot,
                                bool forceTargetChange = false);
  DashboardSnapshot effectiveSnapshot() const;
  int16_t temperatureToCx10(float temperatureC) const;
  float lowPassFilter(float previous, float current, float alpha) const;

  WebServer *server_ = nullptr;
  NetworkManager *network_ = nullptr;
  DashboardSnapshot snapshot_;
  DevDashboardState devState_;
  OtaStatus otaStatus_;
  CoolingConfig coolingConfig_;
  CoolingConfig pendingCoolingConfig_;
  NetworkConfig pendingNetworkConfig_;
  bool hasPendingConfig_ = false;
  TemperatureHistory temperatureHistory_;
  uint32_t lastTemperatureHistorySampleMs_ = 0U;
  bool hasTemperatureHistorySample_ = false;
  float liveFilteredTemperatureC_ = 0.0F;
  float storedFilteredTemperatureC_ = 0.0F;
  bool hasLiveFilteredTemperature_ = false;
  bool hasStoredFilteredTemperature_ = false;
  int16_t lastStoredTemperatureCx10_ = 0;
  int16_t lastStoredTargetCx10_ = 0;
  bool lastStoredSensorDisconnected_ = false;
};
