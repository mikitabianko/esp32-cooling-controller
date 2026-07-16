#include "network/NetworkManager.h"

#include <WiFi.h>
#include <cstring>

#include "debug/DebugLog.h"
#include "web/JsonHelpers.h"

namespace {

constexpr unsigned long kNetworkScanCacheMs = 30000;
constexpr unsigned long kNetworkScanFailureCacheMs = 5000;
constexpr unsigned long kStationConnectPrepareMs = 250;

struct StationAccessPoint {
  bool found = false;
  int channel = 0;
  int rssi = 0;
  uint8_t bssid[6] = {};
  String bssidText;
};

bool isBestNetworkForSsid(int networkIndex, int networkCount)
{
  const String ssid = WiFi.SSID(networkIndex);
  if (ssid.length() == 0) {
    return true;
  }

  const int rssi = WiFi.RSSI(networkIndex);
  for (int i = 0; i < networkCount; ++i) {
    if (i == networkIndex || WiFi.SSID(i) != ssid) {
      continue;
    }
    const int candidateRssi = WiFi.RSSI(i);
    if (candidateRssi > rssi ||
        (candidateRssi == rssi && i < networkIndex)) {
      return false;
    }
  }
  return true;
}

int uniqueNetworkCount(int networkCount)
{
  int count = 0;
  for (int i = 0; i < networkCount; ++i) {
    if (isBestNetworkForSsid(i, networkCount)) {
      ++count;
    }
  }
  return count;
}

bool findStrongestStationAccessPoint(const String &ssid,
                                     StationAccessPoint &accessPoint)
{
  WiFi.scanDelete();
  const int networkCount = WiFi.scanNetworks(false, true, false, 300);
  if (networkCount < 0) {
    WiFi.scanDelete();
    return false;
  }

  for (int i = 0; i < networkCount; ++i) {
    if (WiFi.SSID(i) != ssid) {
      continue;
    }
    const int rssi = WiFi.RSSI(i);
    if (accessPoint.found && rssi <= accessPoint.rssi) {
      continue;
    }
    const uint8_t *bssid = WiFi.BSSID(i);
    if (bssid == nullptr) {
      continue;
    }
    accessPoint.found = true;
    accessPoint.channel = WiFi.channel(i);
    accessPoint.rssi = rssi;
    accessPoint.bssidText = WiFi.BSSIDstr(i);
    std::memcpy(accessPoint.bssid, bssid, sizeof(accessPoint.bssid));
  }

  WiFi.scanDelete();
  return accessPoint.found;
}

} // namespace

NetworkManager::NetworkManager(const NetworkManagerOptions &options)
    : options_(options)
{
}

IPAddress NetworkManager::begin()
{
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.softAP(options_.accessPointSsid.c_str(),
              options_.accessPointPassword.c_str());
  accessPointIp_ = WiFi.softAPIP();
  networkStarted_ = true;
  connectStation();

  DEBUG_PRINT("WiFi AP: ");
  DEBUG_PRINTLN(options_.accessPointSsid);
  DEBUG_PRINT("AP web server: http://");
  DEBUG_PRINTLN(accessPointIp_);
  if (config_.stationSsid.length() == 0) {
    DEBUG_PRINTLN("STA disabled: StationSsid is empty");
  } else if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINT("STA connected to: ");
    DEBUG_PRINTLN(config_.stationSsid);
    DEBUG_PRINT("STA web server: http://");
    DEBUG_PRINTLN(WiFi.localIP());
  } else if (stationConnecting_ || stationConnectPending_) {
    DEBUG_PRINT("STA connection started: ");
    DEBUG_PRINTLN(config_.stationSsid);
  } else {
    DEBUG_PRINT("STA connection failed: ");
    DEBUG_PRINTLN(config_.stationSsid);
  }
  return accessPointIp_;
}

void NetworkManager::update()
{
  updateStationConnection();
  maintainStationConnection();
  updateNetworkScan();
}

void NetworkManager::setConfig(const NetworkConfig &config,
                               bool credentialTrial)
{
  const String previousSsid = config_.stationSsid;
  const String previousPassword = config_.stationPassword;
  config_ = sanitizeNetworkConfig(config);
  credentialTrial_ = credentialTrial;
  credentialResult_ = NetworkCredentialResult::None;
  if (!credentialTrial) {
    persistedConfig_ = config_;
  }

  const bool stationForgotten =
      previousSsid.length() > 0 && config_.stationSsid.length() == 0;
  if (stationForgotten) {
    forgetStationNetwork();
  } else if (networkStarted_ &&
             (config_.stationSsid != previousSsid ||
              config_.stationPassword != previousPassword)) {
    connectStation();
  }
}

const NetworkConfig &NetworkManager::config() const
{
  return config_;
}

const NetworkConfig &NetworkManager::persistedConfig() const
{
  return persistedConfig_;
}

NetworkStatus NetworkManager::status() const
{
  NetworkStatus result;
  result.accessPointIp = accessPointIp_;
  result.accessPointSsid = options_.accessPointSsid;
  result.stationSsid = config_.stationSsid;
  result.stationConnected = WiFi.status() == WL_CONNECTED;
  result.stationState = stationStatusText();
  result.stationLastFailure = lastStationFailure_;
  result.stationIp = result.stationConnected ? WiFi.localIP() : IPAddress();
  result.stationRssi = result.stationConnected ? WiFi.RSSI() : 0;
  return result;
}

String NetworkManager::networksJson()
{
  updateNetworkScan();
  if (networkScanRunning_) {
    return networkScanJson("scanning", false, WIFI_SCAN_RUNNING);
  }
  if (lastNetworkScanOk_ && lastNetworkScanJson_.length() > 0 &&
      millis() - lastNetworkScanMs_ < kNetworkScanCacheMs) {
    return lastNetworkScanJson_;
  }
  if (!lastNetworkScanOk_ && lastNetworkScanJson_.length() > 0 &&
      millis() - lastNetworkScanMs_ < kNetworkScanFailureCacheMs) {
    return lastNetworkScanJson_;
  }
  if (startNetworkScan()) {
    return networkScanJson("scanning", false, WIFI_SCAN_RUNNING);
  }
  return lastNetworkScanJson_.length() > 0
             ? lastNetworkScanJson_
             : networkScanJson("failed", false, WIFI_SCAN_FAILED);
}

NetworkCredentialResult NetworkManager::takeCredentialResult()
{
  const NetworkCredentialResult result = credentialResult_;
  credentialResult_ = NetworkCredentialResult::None;
  return result;
}

void NetworkManager::setStartupProgressHandler(StartupProgressHandler handler,
                                               void *context)
{
  startupProgressHandler_ = handler;
  startupProgressContext_ = context;
}

void NetworkManager::notifyStartupProgress(const char *message,
                                           unsigned long elapsedMs,
                                           unsigned long totalMs)
{
  if (startupProgressHandler_ != nullptr) {
    startupProgressHandler_(startupProgressContext_, message, elapsedMs, totalMs);
  }
}

void NetworkManager::forgetStationNetwork()
{
  credentialTrial_ = false;
  stationConnectPending_ = false;
  stationConnecting_ = false;
  stationReconnectAfterScan_ = false;
  lastStationFailure_ = "";
  if (!networkStarted_) {
    return;
  }
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, true);
  WiFi.mode(WIFI_AP_STA);
  notifyStartupProgress("WiFi STA forgotten", 0, 0);
  DEBUG_PRINTLN("STA network forgotten");
}

void NetworkManager::connectStation()
{
  if (!networkStarted_) {
    return;
  }
  stationConnecting_ = false;
  stationConnectPending_ = false;
  if (networkScanRunning_) {
    stationReconnectAfterScan_ = config_.stationSsid.length() > 0;
    WiFi.setAutoReconnect(false);
    return;
  }
  stationReconnectAfterScan_ = false;
  prepareStationConnect();
}

void NetworkManager::prepareStationConnect()
{
  WiFi.disconnect(false, false);
  if (config_.stationSsid.length() == 0) {
    notifyStartupProgress("WiFi STA disabled", 0, 0);
    return;
  }
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(false);
  stationConnectPending_ = true;
  stationConnectPrepareUntilMs_ = millis() + kStationConnectPrepareMs;
}

void NetworkManager::startStationConnect()
{
  if (!stationConnectPending_) {
    return;
  }
  lastStationConnectAttemptMs_ = millis();
  stationConnectStartedMs_ = lastStationConnectAttemptMs_;
  lastStationProgressMs_ = stationConnectStartedMs_;
  stationConnectPending_ = false;
  stationConnecting_ = true;
  DEBUG_PRINT("STA connecting to: ");
  DEBUG_PRINTLN(config_.stationSsid);
  lastStationFailure_ = "";
  notifyStartupProgress("Connecting WiFi", 0,
                        options_.stationConnectTimeoutMs);

  StationAccessPoint accessPoint;
  if (findStrongestStationAccessPoint(config_.stationSsid, accessPoint)) {
    DEBUG_PRINT("STA strongest AP: ");
    DEBUG_PRINT(accessPoint.bssidText);
    DEBUG_PRINT(" ch ");
    DEBUG_PRINT(accessPoint.channel);
    DEBUG_PRINT(" / ");
    DEBUG_PRINT(accessPoint.rssi);
    DEBUG_PRINTLN(" dBm");
    WiFi.begin(config_.stationSsid.c_str(),
               config_.stationPassword.length() == 0
                   ? nullptr
                   : config_.stationPassword.c_str(),
               accessPoint.channel,
               accessPoint.bssid,
               true);
  } else if (config_.stationPassword.length() == 0) {
    DEBUG_PRINTLN("STA strongest AP not found; using default connect");
    WiFi.begin(config_.stationSsid.c_str());
  } else {
    DEBUG_PRINTLN("STA strongest AP not found; using default connect");
    WiFi.begin(config_.stationSsid.c_str(), config_.stationPassword.c_str());
  }
}

void NetworkManager::updateStationConnection()
{
  if (stationConnectPending_) {
    if (static_cast<long>(millis() - stationConnectPrepareUntilMs_) < 0) {
      return;
    }
    startStationConnect();
  }
  if (!stationConnecting_) {
    return;
  }

  const unsigned long now = millis();
  const unsigned long elapsedMs = now - stationConnectStartedMs_;
  const wl_status_t wifiStatus = WiFi.status();
  if (wifiStatus != WL_CONNECTED &&
      elapsedMs < options_.stationConnectTimeoutMs) {
    if (now - lastStationProgressMs_ >= 500) {
      lastStationProgressMs_ = now;
      notifyStartupProgress("Connecting WiFi", elapsedMs,
                            options_.stationConnectTimeoutMs);
    }
    return;
  }

  stationConnecting_ = false;
  DEBUG_PRINT("STA status: ");
  DEBUG_PRINTLN(stationStatusText());
  if (wifiStatus == WL_CONNECTED) {
    notifyStartupProgress("WiFi connected", elapsedMs,
                          options_.stationConnectTimeoutMs);
    DEBUG_PRINT("STA IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
    lastStationFailure_ = "";
    if (credentialTrial_) {
      credentialTrial_ = false;
      persistedConfig_ = config_;
      credentialResult_ = NetworkCredentialResult::Confirmed;
    }
    return;
  }

  lastStationFailure_ = stationFailureText(wifiStatus, elapsedMs);
  notifyStartupProgress("WiFi not connected", elapsedMs,
                        options_.stationConnectTimeoutMs);
  DEBUG_PRINT("STA connection failed for: ");
  DEBUG_PRINT(config_.stationSsid);
  DEBUG_PRINT(" (");
  DEBUG_PRINT(lastStationFailure_);
  DEBUG_PRINT(", elapsed ");
  DEBUG_PRINT(elapsedMs);
  DEBUG_PRINTLN(" ms)");
  if (credentialTrial_) {
    credentialTrial_ = false;
    config_ = persistedConfig_;
    credentialResult_ = NetworkCredentialResult::Rejected;
    if (config_.stationSsid.length() > 0) {
      connectStation();
    }
  }
}

void NetworkManager::maintainStationConnection()
{
  if (!networkStarted_ || networkScanRunning_ ||
      config_.stationSsid.length() == 0 || stationConnectPending_ ||
      stationConnecting_ || WiFi.status() == WL_CONNECTED) {
    return;
  }
  if (millis() - lastStationConnectAttemptMs_ <
      options_.stationReconnectIntervalMs) {
    return;
  }
  connectStation();
}

bool NetworkManager::startNetworkScan()
{
  stationReconnectAfterScan_ =
      config_.stationSsid.length() > 0 && WiFi.status() != WL_CONNECTED;
  if (stationReconnectAfterScan_) {
    stationConnecting_ = false;
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false, false);
    delay(50);
  }
  WiFi.mode(WIFI_AP_STA);
  WiFi.scanDelete();
  const int result = WiFi.scanNetworks(true, true, false, 300);
  if (result == WIFI_SCAN_RUNNING) {
    networkScanRunning_ = true;
    networkScanStartedMs_ = millis();
    return true;
  }
  if (result >= 0) {
    lastNetworkScanJson_ = completedNetworkScanJson(result);
    lastNetworkScanMs_ = millis();
    lastNetworkScanOk_ = true;
    WiFi.scanDelete();
    resumeStationAfterScan();
    return false;
  }
  networkScanRunning_ = false;
  lastNetworkScanJson_ =
      networkScanJson(wifiScanStatusText(result).c_str(), false, result);
  lastNetworkScanMs_ = millis();
  lastNetworkScanOk_ = false;
  WiFi.scanDelete();
  resumeStationAfterScan();
  return false;
}

void NetworkManager::updateNetworkScan()
{
  if (!networkScanRunning_) {
    return;
  }
  const int result = WiFi.scanComplete();
  if (result == WIFI_SCAN_RUNNING) {
    if (millis() - networkScanStartedMs_ > 12000) {
      networkScanRunning_ = false;
      lastNetworkScanJson_ = networkScanJson("timeout", false, WIFI_SCAN_FAILED);
      lastNetworkScanMs_ = millis();
      lastNetworkScanOk_ = false;
      WiFi.scanDelete();
      resumeStationAfterScan();
    }
    return;
  }
  networkScanRunning_ = false;
  if (result >= 0) {
    lastNetworkScanJson_ = completedNetworkScanJson(result);
    lastNetworkScanOk_ = true;
  } else {
    lastNetworkScanJson_ =
        networkScanJson(wifiScanStatusText(result).c_str(), false, result);
    lastNetworkScanOk_ = false;
  }
  lastNetworkScanMs_ = millis();
  WiFi.scanDelete();
  resumeStationAfterScan();
}

void NetworkManager::resumeStationAfterScan()
{
  WiFi.setAutoReconnect(true);
  if (stationReconnectAfterScan_) {
    stationReconnectAfterScan_ = false;
    connectStation();
  }
}

String NetworkManager::networkScanJson(const char *scanStatus,
                                       bool ok,
                                       int result) const
{
  String json;
  json.reserve(120);
  json += "{\"ok\":";
  json += JsonHelpers::boolText(ok);
  json += ",\"scanResult\":";
  json += result;
  json += ",\"scanStatus\":";
  json += JsonHelpers::escapedString(scanStatus);
  json += ",\"networks\":[],\"count\":0}";
  return json;
}

String NetworkManager::completedNetworkScanJson(int networkCount) const
{
  const int visibleNetworkCount =
      networkCount > 0 ? uniqueNetworkCount(networkCount) : 0;
  String json;
  json.reserve(180 + (visibleNetworkCount > 0 ? visibleNetworkCount * 96 : 0));
  json += "{\"ok\":true";
  json += ",\"scanResult\":";
  json += networkCount;
  json += ",\"scanStatus\":\"complete\"";
  json += ",\"networks\":[";
  int emittedNetworkCount = 0;
  for (int i = 0; i < networkCount; ++i) {
    if (!isBestNetworkForSsid(i, networkCount)) {
      continue;
    }
    if (emittedNetworkCount > 0) {
      json += ",";
    }
    json += "{\"ssid\":";
    json += JsonHelpers::escapedString(WiFi.SSID(i));
    json += ",\"rssi\":";
    json += WiFi.RSSI(i);
    json += ",\"channel\":";
    json += WiFi.channel(i);
    json += ",\"encrypted\":";
    json += JsonHelpers::boolText(WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    json += "}";
    ++emittedNetworkCount;
  }
  json += "],\"count\":";
  json += emittedNetworkCount;
  json += "}";
  return json;
}

String NetworkManager::wifiScanStatusText(int scanResult) const
{
  if (scanResult >= 0) {
    return "complete";
  }
  if (scanResult == WIFI_SCAN_RUNNING) {
    return "running";
  }
  return scanResult == WIFI_SCAN_FAILED ? "failed" : "unknown";
}

String NetworkManager::stationStatusText() const
{
  if (config_.stationSsid.length() == 0) {
    return "disabled";
  }
  switch (WiFi.status()) {
  case WL_CONNECTED:
    return "connected";
  case WL_NO_SSID_AVAIL:
    return "ssid not found";
  case WL_CONNECT_FAILED:
    return "connect failed";
  case WL_CONNECTION_LOST:
    return "connection lost";
  case WL_DISCONNECTED:
    return stationConnecting_ ? "connecting" : "disconnected";
  case WL_IDLE_STATUS:
    return "connecting";
  default:
    return "unknown";
  }
}

String NetworkManager::stationFailureText(int wifiStatus,
                                          unsigned long elapsedMs) const
{
  switch (wifiStatus) {
  case WL_NO_SSID_AVAIL:
    return "SSID not found";
  case WL_CONNECT_FAILED:
    return "authentication or association failed";
  case WL_CONNECTION_LOST:
    return "connection lost";
  case WL_DISCONNECTED:
  case WL_IDLE_STATUS:
    return elapsedMs >= options_.stationConnectTimeoutMs
               ? "connection timeout"
               : "disconnected";
  default:
    return "unknown WiFi status " + String(wifiStatus);
  }
}
