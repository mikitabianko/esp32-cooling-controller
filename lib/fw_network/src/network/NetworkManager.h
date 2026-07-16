#pragma once

#include <Arduino.h>
#include <IPAddress.h>

#include "config/NetworkConfig.h"

struct NetworkManagerOptions {
  String accessPointSsid;
  String accessPointPassword;
  uint32_t stationConnectTimeoutMs = 15000U;
  uint32_t stationReconnectIntervalMs = 30000U;
};

struct NetworkStatus {
  IPAddress accessPointIp;
  String accessPointSsid;
  String stationSsid;
  bool stationConnected = false;
  String stationState;
  String stationLastFailure;
  IPAddress stationIp;
  int stationRssi = 0;
};

enum class NetworkCredentialResult {
  None,
  Confirmed,
  Rejected,
};

class NetworkManager {
public:
  using StartupProgressHandler = void (*)(void *context,
                                          const char *message,
                                          unsigned long elapsedMs,
                                          unsigned long totalMs);

  explicit NetworkManager(const NetworkManagerOptions &options);

  IPAddress begin();
  void update();
  void setConfig(const NetworkConfig &config, bool credentialTrial = false);
  const NetworkConfig &config() const;
  const NetworkConfig &persistedConfig() const;
  NetworkStatus status() const;
  String networksJson();
  NetworkCredentialResult takeCredentialResult();
  void setStartupProgressHandler(StartupProgressHandler handler,
                                 void *context);

private:
  void notifyStartupProgress(const char *message,
                             unsigned long elapsedMs,
                             unsigned long totalMs);
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
  String wifiScanStatusText(int scanResult) const;
  String stationStatusText() const;
  String stationFailureText(int status, unsigned long elapsedMs) const;

  IPAddress accessPointIp_;
  NetworkManagerOptions options_;
  NetworkConfig config_;
  NetworkConfig persistedConfig_;
  bool credentialTrial_ = false;
  NetworkCredentialResult credentialResult_ = NetworkCredentialResult::None;
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
};
