#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#include <WebServer.h>
#include "domain/AppSettings.h"
#include "domain/CoolingController.h"

struct DashboardSnapshot {
  float temperatureC = 0.0F;
  bool hasTemperature = false;
  bool sensorDisconnected = false;
  int updateCount = 0;
  CoolingState coolingState;
  AppSettings settings;
  unsigned long uptimeMs = 0;
};

struct TemperatureHistorySample {
  unsigned long uptimeMs = 0;
  int16_t temperatureCx10 = 0;
  int16_t targetCx10 = 0;
  uint8_t flags = 0;
};

class WebDashboard {
public:
  using StartupProgressHandler = void (*)(void *context,
                                          const char *message,
                                          unsigned long elapsedMs,
                                          unsigned long totalMs);

  WebDashboard();

  IPAddress begin();
  void update();
  void setStartupProgressHandler(StartupProgressHandler handler,
                                 void *context);
  void setSnapshot(const DashboardSnapshot &snapshot);
  void setSettings(const AppSettings &settings);
  bool takePendingSettings(AppSettings &settings);
  IPAddress ipAddress() const;

private:
  static constexpr size_t kTemperatureHistoryCapacity = 720;
  static constexpr unsigned long kTemperatureHistoryKeepaliveMs = 30000;
  static constexpr int16_t kTemperatureHistoryChangeCx10 = 1;
  static constexpr uint8_t kTemperatureHistoryDisconnectedFlag = 1U;

  void handlePage();
  void handleStatus();
  void handleHistory();
  void handleHistoryCsv();
  void handleGetSettings();
  void handleSaveSettings();
  void handleNetworks();
  void handleNotFound();
  String statusJson() const;
  String historyJson() const;
  String historyCsv() const;
  String settingsJson() const;
  String networksJson();
  String jsonString(const String &value) const;
  String boolText(bool value) const;
  String wifiScanStatusText(int scanResult) const;
  String stationStatusText() const;
  String stationFailureText(int status, unsigned long elapsedMs) const;
  void notifyStartupProgress(const char *message,
                             unsigned long elapsedMs,
                             unsigned long totalMs);
  void queueSettingsSave(const AppSettings &settings);
  void saveConfirmedStationCredentials();
  void discardUnconfirmedStationCredentials(int status,
                                            unsigned long elapsedMs);
  void forgetStationNetwork();
  void connectStation();
  void prepareStationConnect();
  void startStationConnect();
  void updateStationConnection();
  void maintainStationConnection();
  bool startNetworkScan();
  void updateNetworkScan();
  void resumeStationAfterScan();
  String networkScanJson(const char *status, bool ok, int result) const;
  String completedNetworkScanJson(int networkCount) const;
  bool readSettingsArgs(AppSettings &settings);
  bool readBoolArg(const char *name, bool &value);
  bool readStringArg(const char *name, String &value);
  bool readIntArg(const char *name, int &value);
  bool readFloatArg(const char *name, float &value);
  bool readUnsignedLongArg(const char *name, unsigned long &value);
  void recordTargetChange(unsigned long uptimeMs);
  void recordTemperatureHistory(const DashboardSnapshot &snapshot,
                                bool forceTargetChange = false);
  void appendTemperatureHistorySample(const TemperatureHistorySample &sample);
  int16_t temperatureToCx10(float temperatureC) const;
  float lowPassFilter(float previous, float current, float alpha) const;

  WebServer server_;
  IPAddress ipAddress_;
  DashboardSnapshot snapshot_;
  AppSettings settings_;
  AppSettings persistedSettings_;
  AppSettings pendingSettings_;
  bool hasPendingSettings_ = false;
  bool stationCredentialSavePending_ = false;
  bool networkStarted_ = false;
  StartupProgressHandler startupProgressHandler_ = nullptr;
  void *startupProgressContext_ = nullptr;
  unsigned long lastStationConnectAttemptMs_ = 0;
  unsigned long stationConnectStartedMs_ = 0;
  unsigned long lastStationProgressMs_ = 0;
  unsigned long stationConnectPrepareUntilMs_ = 0;
  bool stationConnecting_ = false;
  bool stationConnectPending_ = false;
  bool networkScanRunning_ = false;
  bool stationReconnectAfterScan_ = false;
  String lastStationFailure_;
  unsigned long networkScanStartedMs_ = 0;
  unsigned long lastNetworkScanMs_ = 0;
  bool lastNetworkScanOk_ = false;
  String lastNetworkScanJson_;
  TemperatureHistorySample temperatureHistory_[kTemperatureHistoryCapacity];
  size_t temperatureHistoryStart_ = 0;
  size_t temperatureHistoryCount_ = 0;
  unsigned long lastTemperatureHistorySampleMs_ = 0;
  bool hasTemperatureHistorySample_ = false;
  float liveFilteredTemperatureC_ = 0.0F;
  float storedFilteredTemperatureC_ = 0.0F;
  bool hasLiveFilteredTemperature_ = false;
  bool hasStoredFilteredTemperature_ = false;
  int16_t lastStoredTemperatureCx10_ = 0;
  int16_t lastStoredTargetCx10_ = 0;
  bool lastStoredSensorDisconnected_ = false;
};
