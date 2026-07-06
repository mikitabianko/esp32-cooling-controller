#include "web/WebDashboard.h"

#include <Update.h>
#include <WiFi.h>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "config/Config.h"
#include "debug/DebugLog.h"
#include "domain/TemperatureLogic.h"
#include "web/WebDashboardPage.h"

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

bool isSensorDisconnected(float temperatureC)
{
  return classifyTemperature(temperatureC) == TemperatureStatus::Disconnected;
}

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

WebDashboard::WebDashboard()
    : server_(Config::Network::WebServerPort)
{
}

IPAddress WebDashboard::begin()
{
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  WiFi.softAP(Config::Network::AccessPointSsid,
              Config::Network::AccessPointPassword);
  ipAddress_ = WiFi.softAPIP();
  networkStarted_ = true;

  server_.on("/", [this]() { handlePage(); });
  server_.on("/api/status", [this]() { handleStatus(); });
  server_.on("/api/history", HTTP_GET, [this]() { handleHistory(); });
  server_.on("/api/history.csv", HTTP_GET, [this]() { handleHistoryCsv(); });
  server_.on("/api/dev", HTTP_GET, [this]() { handleGetDevState(); });
  server_.on("/api/dev", HTTP_POST, [this]() { handleSaveDevState(); });
  server_.on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
  server_.on("/api/settings", HTTP_POST, [this]() { handleSaveSettings(); });
  server_.on("/api/networks", HTTP_GET, [this]() { handleNetworks(); });
  server_.on(
      "/api/firmware",
      HTTP_POST,
      [this]() { handleFirmwareUploadComplete(); },
      [this]() { handleFirmwareUploadStream(); });
  server_.onNotFound([this]() { handleNotFound(); });
  server_.begin();
  connectStation();

  DEBUG_PRINT("WiFi AP: ");
  DEBUG_PRINTLN(Config::Network::AccessPointSsid);
  DEBUG_PRINT("AP web server: http://");
  DEBUG_PRINTLN(ipAddress_);
  if (settings_.stationSsid.length() > 0) {
    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINT("STA connected to: ");
      DEBUG_PRINTLN(settings_.stationSsid);
      DEBUG_PRINT("STA web server: http://");
      DEBUG_PRINTLN(WiFi.localIP());
    } else if (stationConnecting_) {
      DEBUG_PRINT("STA connection started: ");
      DEBUG_PRINTLN(settings_.stationSsid);
    } else {
      DEBUG_PRINT("STA connection failed: ");
      DEBUG_PRINTLN(settings_.stationSsid);
    }
  } else {
    DEBUG_PRINTLN("STA disabled: StationSsid is empty");
  }

  return ipAddress_;
}

void WebDashboard::update()
{
  updateStationConnection();
  maintainStationConnection();
  updateNetworkScan();
  server_.handleClient();
  if (firmwareRestartPending_ &&
      static_cast<long>(millis() - firmwareRestartAtMs_) >= 0) {
    ESP.restart();
  }
}

void WebDashboard::setStartupProgressHandler(StartupProgressHandler handler,
                                             void *context)
{
  startupProgressHandler_ = handler;
  startupProgressContext_ = context;
}

void WebDashboard::setSnapshot(const DashboardSnapshot &snapshot)
{
  snapshot_ = snapshot;
  snapshot_.settings = settings_;
  snapshot_.sensorDisconnected =
      snapshot_.hasTemperature && isSensorDisconnected(snapshot_.temperatureC);
  if (snapshot_.hasTemperature && !snapshot_.sensorDisconnected) {
    snapshot_.temperatureC = hasLiveFilteredTemperature_
                                 ? lowPassFilter(liveFilteredTemperatureC_,
                                                 snapshot_.temperatureC,
                                                 0.65F)
                                 : snapshot_.temperatureC;
    liveFilteredTemperatureC_ = snapshot_.temperatureC;
    hasLiveFilteredTemperature_ = true;
  }
  recordTemperatureHistory(snapshot_);
}

void WebDashboard::setSettings(const AppSettings &settings)
{
  const String previousSsid = settings_.stationSsid;
  const String previousPassword = settings_.stationPassword;
  settings_ = sanitizedSettings(settings);
  persistedSettings_ = settings_;
  snapshot_.settings = settings_;
  const bool stationForgotten =
      previousSsid.length() > 0 && settings_.stationSsid.length() == 0;
  if (stationForgotten) {
    forgetStationNetwork();
  } else if (networkStarted_ &&
             (settings_.stationSsid != previousSsid ||
              settings_.stationPassword != previousPassword)) {
    connectStation();
  }
}

bool WebDashboard::takePendingSettings(AppSettings &settings)
{
  if (!hasPendingSettings_) {
    return false;
  }

  settings = pendingSettings_;
  hasPendingSettings_ = false;
  return true;
}

void WebDashboard::setOtaStatus(const OtaStatus &status)
{
  otaStatus_ = status;
}

IPAddress WebDashboard::ipAddress() const
{
  return ipAddress_;
}

void WebDashboard::handlePage()
{
  const char *page = webPageForPath(server_.uri());
  if (page == nullptr) {
    server_.send(404, "text/plain", "Not found");
    return;
  }
  server_.send_P(200, "text/html; charset=utf-8", page);
}

void WebDashboard::handleStatus()
{
  server_.send(200, "application/json", statusJson());
}

void WebDashboard::handleHistory()
{
  server_.send(200, "application/json", historyJson());
}

void WebDashboard::handleHistoryCsv()
{
  server_.send(200, "text/csv; charset=utf-8", historyCsv());
}

void WebDashboard::handleGetDevState()
{
  server_.send(200, "application/json", devJson());
}

void WebDashboard::handleSaveDevState()
{
  DevDashboardState state;
  if (!readDevArgs(state)) {
    server_.send(400,
                 "application/json",
                 "{\"ok\":false,\"error\":\"invalid dev state\"}");
    return;
  }

  devState_ = state;
  server_.send(200, "application/json", devJson());
}

void WebDashboard::handleGetSettings()
{
  server_.send(200, "application/json", settingsJson());
}

void WebDashboard::handleSaveSettings()
{
  AppSettings settings;
  if (!readSettingsArgs(settings)) {
    server_.send(400,
                 "application/json",
                 "{\"ok\":false,\"error\":\"invalid settings\"}");
    return;
  }

  const AppSettings previousSettings = settings_;
  const String previousSsid = settings_.stationSsid;
  const String previousPassword = settings_.stationPassword;
  settings_ = sanitizedSettings(settings);
  snapshot_.settings = settings_;
  const bool targetChanged =
      std::fabs(settings_.targetTemperatureC -
                previousSettings.targetTemperatureC) >= 0.05F;
  const bool stationChanged = settings_.stationSsid != previousSsid ||
                              settings_.stationPassword != previousPassword;
  const bool stationForgotten =
      previousSsid.length() > 0 && settings_.stationSsid.length() == 0;
  const bool stationNeedsTrial = stationChanged &&
                                 settings_.stationSsid.length() > 0;

  if (stationNeedsTrial) {
    AppSettings settingsToSave = settings_;
    settingsToSave.stationSsid = persistedSettings_.stationSsid;
    settingsToSave.stationPassword = persistedSettings_.stationPassword;
    stationCredentialSavePending_ = true;
    queueSettingsSave(settingsToSave);
    connectStation();
  } else {
    stationCredentialSavePending_ = false;
    persistedSettings_ = settings_;
    queueSettingsSave(settings_);
    if (stationForgotten) {
      forgetStationNetwork();
    } else if (stationChanged) {
      connectStation();
    }
  }
  if (targetChanged) {
    recordTargetChange(millis());
  }
  server_.send(200, "application/json", settingsJson());
}

void WebDashboard::handleNetworks()
{
  server_.send(200, "application/json", networksJson());
}

void WebDashboard::handleFirmwareUploadComplete()
{
  if (!firmwareUploadOk_) {
    String json = "{\"ok\":false,\"error\":";
    json += jsonString(firmwareUploadError_.length() > 0
                           ? firmwareUploadError_
                           : String("firmware upload failed"));
    json += "}";
    server_.send(500, "application/json", json);
    return;
  }

  firmwareRestartPending_ = true;
  firmwareRestartAtMs_ = millis() + 1200;
  server_.send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
}

void WebDashboard::handleFirmwareUploadStream()
{
  HTTPUpload &upload = server_.upload();

  if (upload.status == UPLOAD_FILE_START) {
    firmwareUploadOk_ = false;
    firmwareUploadError_ = "";
    DEBUG_PRINT("HTTP firmware update started: ");
    DEBUG_PRINTLN(upload.filename);
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      firmwareUploadError_ = Update.errorString();
      DEBUG_PRINT("HTTP firmware update failed: ");
      DEBUG_PRINTLN(firmwareUploadError_);
      return;
    }
    firmwareUploadOk_ = true;
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (!firmwareUploadOk_) {
      return;
    }
    const size_t written = Update.write(upload.buf, upload.currentSize);
    if (written != upload.currentSize) {
      firmwareUploadOk_ = false;
      firmwareUploadError_ = Update.errorString();
      Update.abort();
      DEBUG_PRINT("HTTP firmware update failed: ");
      DEBUG_PRINTLN(firmwareUploadError_);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (!firmwareUploadOk_) {
      return;
    }
    if (!Update.end(true)) {
      firmwareUploadOk_ = false;
      firmwareUploadError_ = Update.errorString();
      DEBUG_PRINT("HTTP firmware update failed: ");
      DEBUG_PRINTLN(firmwareUploadError_);
      return;
    }
    DEBUG_PRINT("HTTP firmware update completed: ");
    DEBUG_PRINT(upload.totalSize);
    DEBUG_PRINTLN(" bytes");
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    firmwareUploadOk_ = false;
    firmwareUploadError_ = "upload aborted";
    Update.abort();
    DEBUG_PRINTLN("HTTP firmware update failed: upload aborted");
  }
}

void WebDashboard::handleNotFound()
{
  const char *page = webPageForPath(server_.uri());
  if (page != nullptr) {
    server_.send_P(200, "text/html; charset=utf-8", page);
    return;
  }
  server_.send(404, "text/plain", "Not found");
}

String WebDashboard::statusJson() const
{
  const DashboardSnapshot snapshot = effectiveSnapshot();
  String json;
  json.reserve(560);
  json += "{";
  json += "\"temperatureC\":";
  json += snapshot.temperatureC;
  json += ",\"filteredTemperatureC\":";
  json += snapshot.temperatureC;
  json += ",\"hasTemperature\":";
  json += boolText(snapshot.hasTemperature);
  json += ",\"sensorDisconnected\":";
  json += boolText(snapshot.sensorDisconnected);
  json += ",\"updateCount\":";
  json += snapshot.updateCount;
  json += ",\"peltierRunning\":";
  json += boolText(snapshot.coolingState.peltierRunning);
  json += ",\"fanRunning\":";
  json += boolText(snapshot.coolingState.fanRunning);
  json += ",\"fanRunOnActive\":";
  json += boolText(snapshot.coolingState.fanRunOnActive);
  json += ",\"fanRunOnRemainingMs\":";
  json += snapshot.coolingState.fanRunOnRemainingMs;
  json += ",\"targetC\":";
  json += settings_.targetTemperatureC;
  json += ",\"hysteresisC\":";
  json += settings_.hysteresisC;
  json += ",\"measurementIntervalMs\":";
  json += settings_.measurementIntervalMs;
  json += ",\"fanRunOnMs\":";
  json += settings_.fanRunOnMs;
  json += ",\"uptimeMs\":";
  json += snapshot.uptimeMs;
  json += ",\"devMode\":";
  json += boolText(devState_.enabled);
  json += ",\"accessPointSsid\":";
  json += jsonString(Config::Network::AccessPointSsid);
  json += ",\"accessPointIp\":";
  json += jsonString(ipAddress_.toString());
  json += ",\"stationSsid\":";
  json += jsonString(settings_.stationSsid);
  json += ",\"stationConnected\":";
  json += boolText(WiFi.status() == WL_CONNECTED);
  json += ",\"stationStatus\":";
  json += jsonString(stationStatusText());
  json += ",\"stationLastFailure\":";
  json += jsonString(lastStationFailure_);
  json += ",\"stationIp\":";
  json += jsonString(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString()
                                                   : String(""));
  json += ",\"stationRssi\":";
  json += WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  appendOtaJson(json);
  json += "}";
  return json;
}

String WebDashboard::historyJson() const
{
  String json;
  json.reserve(430 + temperatureHistoryCount_ * 42);
  json += "{";
  json += "\"ok\":true";
  json += ",\"capacity\":";
  json += kTemperatureHistoryCapacity;
  json += ",\"keepaliveMs\":";
  json += kTemperatureHistoryKeepaliveMs;
  json += ",\"changeThresholdCx10\":";
  json += kTemperatureHistoryChangeCx10;
  json += ",\"disconnectedFlag\":";
  json += kTemperatureHistoryDisconnectedFlag;
  json += ",\"fields\":[\"uptimeMs\",\"temperatureCx10\",\"targetCx10\",\"flags\",\"sensorValid\",\"sensorDisconnected\"]";
  json += ",\"series\":[{\"id\":\"probe1\",\"label\":\"Filtered temperature\",\"unit\":\"C\",\"points\":[";
  for (size_t i = 0; i < temperatureHistoryCount_; ++i) {
    if (i > 0) {
      json += ",";
    }
    const size_t index =
        (temperatureHistoryStart_ + i) % kTemperatureHistoryCapacity;
    const TemperatureHistorySample &sample = temperatureHistory_[index];
    const bool disconnected =
        (sample.flags & kTemperatureHistoryDisconnectedFlag) != 0;
    json += "[";
    json += sample.uptimeMs;
    json += ",";
    json += sample.temperatureCx10;
    json += ",";
    json += sample.targetCx10;
    json += ",";
    json += sample.flags;
    json += ",";
    json += disconnected ? 0 : 1;
    json += ",";
    json += disconnected ? 1 : 0;
    json += "]";
  }
  json += "]}]}";
  return json;
}

String WebDashboard::historyCsv() const
{
  String csv;
  csv.reserve(96 + temperatureHistoryCount_ * 44);
  csv += "timestamp_ms,filtered_temperature_c,target_temperature_c,sensor_status,sensor_valid,sensor_disconnected,flags\n";
  for (size_t i = 0; i < temperatureHistoryCount_; ++i) {
    const size_t index =
        (temperatureHistoryStart_ + i) % kTemperatureHistoryCapacity;
    const TemperatureHistorySample &sample = temperatureHistory_[index];
    const bool disconnected =
        (sample.flags & kTemperatureHistoryDisconnectedFlag) != 0;
    csv += sample.uptimeMs;
    csv += ",";
    if (!disconnected) {
      csv += String(static_cast<float>(sample.temperatureCx10) / 10.0F, 1);
    }
    csv += ",";
    csv += String(static_cast<float>(sample.targetCx10) / 10.0F, 1);
    csv += ",";
    csv += disconnected ? "unavailable" : "ok";
    csv += ",";
    csv += disconnected ? "0" : "1";
    csv += ",";
    csv += disconnected ? "1" : "0";
    csv += ",";
    csv += sample.flags;
    csv += "\n";
  }
  return csv;
}

String WebDashboard::settingsJson() const
{
  String json;
  json.reserve(280);
  json += "{\"ok\":true";
  json += ",\"targetC\":";
  json += settings_.targetTemperatureC;
  json += ",\"hysteresisC\":";
  json += settings_.hysteresisC;
  json += ",\"measurementIntervalMs\":";
  json += settings_.measurementIntervalMs;
  json += ",\"fanRunOnMs\":";
  json += settings_.fanRunOnMs;
  json += ",\"accessPointSsid\":";
  json += jsonString(Config::Network::AccessPointSsid);
  json += ",\"accessPointIp\":";
  json += jsonString(ipAddress_.toString());
  json += ",\"stationSsid\":";
  json += jsonString(settings_.stationSsid);
  json += ",\"stationPasswordSet\":";
  json += boolText(settings_.stationPassword.length() > 0);
  json += ",\"stationConnected\":";
  json += boolText(WiFi.status() == WL_CONNECTED);
  json += ",\"stationStatus\":";
  json += jsonString(stationStatusText());
  json += ",\"stationLastFailure\":";
  json += jsonString(lastStationFailure_);
  json += ",\"stationIp\":";
  json += jsonString(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString()
                                                   : String(""));
  json += ",\"stationRssi\":";
  json += WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  appendOtaJson(json);
  json += "}";
  return json;
}

void WebDashboard::appendOtaJson(String &json) const
{
  json += ",\"otaEnabled\":";
  json += boolText(otaStatus_.enabled);
  json += ",\"otaStarted\":";
  json += boolText(otaStatus_.started);
  json += ",\"otaUpdating\":";
  json += boolText(otaStatus_.updating);
  json += ",\"otaProgress\":";
  json += otaStatus_.progressPercent;
  json += ",\"otaStatus\":";
  json += jsonString(otaStatus_.state);
  json += ",\"otaHostname\":";
  json += jsonString(Config::Ota::Hostname);
  json += ",\"otaPasswordSet\":";
  json += boolText(strlen(Config::Ota::Password) > 0 ||
                   strlen(Config::Ota::PasswordHash) > 0);
  json += ",\"otaLastError\":";
  json += jsonString(otaStatus_.lastError);
}

String WebDashboard::networksJson()
{
  updateNetworkScan();
  if (networkScanRunning_) {
    return networkScanJson("scanning", false, WIFI_SCAN_RUNNING);
  }
  if (lastNetworkScanOk_ &&
      lastNetworkScanJson_.length() > 0 &&
      millis() - lastNetworkScanMs_ < kNetworkScanCacheMs) {
    return lastNetworkScanJson_;
  }
  if (!lastNetworkScanOk_ &&
      lastNetworkScanJson_.length() > 0 &&
      millis() - lastNetworkScanMs_ < kNetworkScanFailureCacheMs) {
    return lastNetworkScanJson_;
  }
  if (startNetworkScan()) {
    return networkScanJson("scanning", false, WIFI_SCAN_RUNNING);
  }
  if (lastNetworkScanJson_.length() > 0) {
    return lastNetworkScanJson_;
  }
  return networkScanJson("failed", false, WIFI_SCAN_FAILED);
}

String WebDashboard::devJson() const
{
  String json;
  json.reserve(260);
  json += "{\"ok\":true";
  json += ",\"enabled\":";
  json += boolText(devState_.enabled);
  json += ",\"temperatureC\":";
  json += devState_.temperatureC;
  json += ",\"hasTemperature\":";
  json += boolText(devState_.hasTemperature);
  json += ",\"sensorDisconnected\":";
  json += boolText(devState_.sensorDisconnected);
  json += ",\"updateCount\":";
  json += devState_.updateCount;
  json += ",\"peltierRunning\":";
  json += boolText(devState_.coolingState.peltierRunning);
  json += ",\"fanRunning\":";
  json += boolText(devState_.coolingState.fanRunning);
  json += ",\"fanRunOnActive\":";
  json += boolText(devState_.coolingState.fanRunOnActive);
  json += ",\"fanRunOnRemainingMs\":";
  json += devState_.coolingState.fanRunOnRemainingMs;
  json += "}";
  return json;
}

String WebDashboard::jsonString(const String &value) const
{
  String escaped;
  escaped.reserve(value.length() + 2);
  escaped += "\"";
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '"' || c == '\\') {
      escaped += "\\";
    }
    escaped += c;
  }
  escaped += "\"";
  return escaped;
}

String WebDashboard::boolText(bool value) const
{
  return value ? "true" : "false";
}

String WebDashboard::wifiScanStatusText(int scanResult) const
{
  if (scanResult >= 0) {
    return "complete";
  }
  if (scanResult == WIFI_SCAN_RUNNING) {
    return "running";
  }
  if (scanResult == WIFI_SCAN_FAILED) {
    return "failed";
  }
  return "unknown";
}

String WebDashboard::stationStatusText() const
{
  if (settings_.stationSsid.length() == 0) {
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

String WebDashboard::stationFailureText(int status,
                                        unsigned long elapsedMs) const
{
  switch (status) {
  case WL_NO_SSID_AVAIL:
    return "SSID not found";
  case WL_CONNECT_FAILED:
    return "authentication or association failed";
  case WL_CONNECTION_LOST:
    return "connection lost";
  case WL_DISCONNECTED:
  case WL_IDLE_STATUS:
    if (elapsedMs >= Config::Network::StationConnectTimeoutMs) {
      return "connection timeout";
    }
    return "disconnected";
  default:
    return "unknown WiFi status " + String(status);
  }
}

void WebDashboard::notifyStartupProgress(const char *message,
                                         unsigned long elapsedMs,
                                         unsigned long totalMs)
{
  if (startupProgressHandler_ == nullptr) {
    return;
  }

  startupProgressHandler_(startupProgressContext_,
                          message,
                          elapsedMs,
                          totalMs);
}

void WebDashboard::queueSettingsSave(const AppSettings &settings)
{
  pendingSettings_ = sanitizedSettings(settings);
  hasPendingSettings_ = true;
}

void WebDashboard::saveConfirmedStationCredentials()
{
  if (!stationCredentialSavePending_) {
    return;
  }

  stationCredentialSavePending_ = false;
  persistedSettings_ = settings_;
  queueSettingsSave(settings_);
  DEBUG_PRINT("STA credentials confirmed and saved for: ");
  DEBUG_PRINTLN(settings_.stationSsid);
}

void WebDashboard::discardUnconfirmedStationCredentials(int status,
                                                        unsigned long elapsedMs)
{
  if (!stationCredentialSavePending_) {
    return;
  }

  DEBUG_PRINT("STA credentials not saved for: ");
  DEBUG_PRINT(settings_.stationSsid);
  DEBUG_PRINT(" (");
  DEBUG_PRINT(stationFailureText(status, elapsedMs));
  DEBUG_PRINTLN(")");
  stationCredentialSavePending_ = false;
  settings_.stationSsid = persistedSettings_.stationSsid;
  settings_.stationPassword = persistedSettings_.stationPassword;
  snapshot_.settings = settings_;
  if (settings_.stationSsid.length() > 0) {
    connectStation();
  }
}

void WebDashboard::forgetStationNetwork()
{
  stationCredentialSavePending_ = false;
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

void WebDashboard::connectStation()
{
  if (!networkStarted_) {
    return;
  }

  stationConnecting_ = false;
  stationConnectPending_ = false;
  if (networkScanRunning_) {
    stationReconnectAfterScan_ = settings_.stationSsid.length() > 0;
    WiFi.setAutoReconnect(false);
    return;
  }

  stationReconnectAfterScan_ = false;
  prepareStationConnect();
}

void WebDashboard::prepareStationConnect()
{
  WiFi.disconnect(false, false);
  if (settings_.stationSsid.length() == 0) {
    notifyStartupProgress("WiFi STA disabled", 0, 0);
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(false);
  stationConnectPending_ = true;
  stationConnectPrepareUntilMs_ = millis() + kStationConnectPrepareMs;
}

void WebDashboard::startStationConnect()
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
  DEBUG_PRINTLN(settings_.stationSsid);
  lastStationFailure_ = "";
  notifyStartupProgress("Connecting WiFi", 0,
                        Config::Network::StationConnectTimeoutMs);

  StationAccessPoint accessPoint;
  if (findStrongestStationAccessPoint(settings_.stationSsid, accessPoint)) {
    DEBUG_PRINT("STA strongest AP: ");
    DEBUG_PRINT(accessPoint.bssidText);
    DEBUG_PRINT(" ch ");
    DEBUG_PRINT(accessPoint.channel);
    DEBUG_PRINT(" / ");
    DEBUG_PRINT(accessPoint.rssi);
    DEBUG_PRINTLN(" dBm");

    WiFi.begin(settings_.stationSsid.c_str(),
               settings_.stationPassword.length() == 0
                   ? nullptr
                   : settings_.stationPassword.c_str(),
               accessPoint.channel,
               accessPoint.bssid,
               true);
  } else if (settings_.stationPassword.length() == 0) {
    DEBUG_PRINTLN("STA strongest AP not found; using default connect");
    WiFi.begin(settings_.stationSsid.c_str());
  } else {
    DEBUG_PRINTLN("STA strongest AP not found; using default connect");
    WiFi.begin(settings_.stationSsid.c_str(), settings_.stationPassword.c_str());
  }
}

void WebDashboard::updateStationConnection()
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
  const wl_status_t status = WiFi.status();
  if (status != WL_CONNECTED &&
      elapsedMs < Config::Network::StationConnectTimeoutMs) {
    if (now - lastStationProgressMs_ >= 500) {
      lastStationProgressMs_ = now;
      notifyStartupProgress("Connecting WiFi",
                            elapsedMs,
                            Config::Network::StationConnectTimeoutMs);
    }
    return;
  }

  stationConnecting_ = false;
  DEBUG_PRINT("STA status: ");
  DEBUG_PRINTLN(stationStatusText());
  if (status == WL_CONNECTED) {
    notifyStartupProgress("WiFi connected",
                          elapsedMs,
                          Config::Network::StationConnectTimeoutMs);
    DEBUG_PRINT("STA IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
    lastStationFailure_ = "";
    saveConfirmedStationCredentials();
  } else {
    lastStationFailure_ = stationFailureText(status, elapsedMs);
    notifyStartupProgress("WiFi not connected",
                          elapsedMs,
                          Config::Network::StationConnectTimeoutMs);
    DEBUG_PRINT("STA connection failed for: ");
    DEBUG_PRINT(settings_.stationSsid);
    DEBUG_PRINT(" (");
    DEBUG_PRINT(stationFailureText(status, elapsedMs));
    DEBUG_PRINT(", elapsed ");
    DEBUG_PRINT(elapsedMs);
    DEBUG_PRINTLN(" ms)");
    discardUnconfirmedStationCredentials(status, elapsedMs);
  }
}

void WebDashboard::maintainStationConnection()
{
  if (!networkStarted_ || networkScanRunning_ ||
      settings_.stationSsid.length() == 0 ||
      stationConnectPending_ ||
      stationConnecting_ ||
      WiFi.status() == WL_CONNECTED) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastStationConnectAttemptMs_ <
      Config::Network::StationReconnectIntervalMs) {
    return;
  }

  connectStation();
}

bool WebDashboard::startNetworkScan()
{
  stationReconnectAfterScan_ =
      settings_.stationSsid.length() > 0 && WiFi.status() != WL_CONNECTED;
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
  lastNetworkScanJson_ = networkScanJson(wifiScanStatusText(result).c_str(),
                                         false,
                                         result);
  lastNetworkScanMs_ = millis();
  lastNetworkScanOk_ = false;
  WiFi.scanDelete();
  resumeStationAfterScan();
  return false;
}

void WebDashboard::updateNetworkScan()
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
    lastNetworkScanJson_ = networkScanJson(wifiScanStatusText(result).c_str(),
                                           false,
                                           result);
    lastNetworkScanOk_ = false;
  }
  lastNetworkScanMs_ = millis();
  WiFi.scanDelete();
  resumeStationAfterScan();
}

void WebDashboard::resumeStationAfterScan()
{
  WiFi.setAutoReconnect(true);
  if (!stationReconnectAfterScan_) {
    return;
  }

  stationReconnectAfterScan_ = false;
  connectStation();
}

String WebDashboard::networkScanJson(const char *status,
                                     bool ok,
                                     int result) const
{
  String json;
  json.reserve(120);
  json += "{\"ok\":";
  json += boolText(ok);
  json += ",\"scanResult\":";
  json += result;
  json += ",\"scanStatus\":";
  json += jsonString(status);
  json += ",\"networks\":[],\"count\":0}";
  return json;
}

String WebDashboard::completedNetworkScanJson(int networkCount) const
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
  if (visibleNetworkCount > 0) {
    for (int i = 0; i < networkCount; ++i) {
      if (!isBestNetworkForSsid(i, networkCount)) {
        continue;
      }
      if (emittedNetworkCount > 0) {
        json += ",";
      }
      json += "{\"ssid\":";
      json += jsonString(WiFi.SSID(i));
      json += ",\"rssi\":";
      json += WiFi.RSSI(i);
      json += ",\"channel\":";
      json += WiFi.channel(i);
      json += ",\"encrypted\":";
      json += boolText(WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      json += "}";
      ++emittedNetworkCount;
    }
  }
  json += "],\"count\":";
  json += emittedNetworkCount;
  json += "}";
  return json;
}

bool WebDashboard::readSettingsArgs(AppSettings &settings)
{
  float targetC = 0.0F;
  float hysteresisC = 0.0F;
  unsigned long measurementIntervalMs = 0;
  unsigned long fanRunOnMs = 0;
  String stationSsid;
  String stationPassword;
  bool forgetStationNetwork = false;

  if (!readFloatArg("targetC", targetC) ||
      !readFloatArg("hysteresisC", hysteresisC) ||
      !readUnsignedLongArg("measurementIntervalMs", measurementIntervalMs) ||
      !readUnsignedLongArg("fanRunOnMs", fanRunOnMs) ||
      !readStringArg("stationSsid", stationSsid)) {
    return false;
  }
  if (server_.hasArg("stationPassword") &&
      !readStringArg("stationPassword", stationPassword)) {
    return false;
  }
  if (server_.hasArg("forgetStationNetwork") &&
      !readBoolArg("forgetStationNetwork", forgetStationNetwork)) {
    return false;
  }
  if (!forgetStationNetwork && server_.hasArg("clearStationPassword") &&
      !readBoolArg("clearStationPassword", forgetStationNetwork)) {
    return false;
  }

  settings.targetTemperatureC = targetC;
  settings.hysteresisC = hysteresisC;
  settings.measurementIntervalMs = measurementIntervalMs;
  settings.fanRunOnMs = fanRunOnMs;
  settings.stationPassword = settings_.stationPassword;
  const bool stationSsidChanged = stationSsid != settings_.stationSsid;
  if (forgetStationNetwork) {
    stationSsid = "";
    stationPassword = "";
  }
  settings.stationSsid = stationSsid;
  if (forgetStationNetwork || stationSsid.length() == 0 ||
      (stationSsidChanged && stationPassword.length() == 0)) {
    settings.stationPassword = "";
  } else if (stationPassword.length() > 0) {
    settings.stationPassword = stationPassword;
  }
  settings = sanitizedSettings(settings);
  return true;
}

bool WebDashboard::readDevArgs(DevDashboardState &state)
{
  bool enabled = false;
  float temperatureC = 0.0F;
  bool hasTemperature = false;
  bool sensorDisconnected = false;
  bool peltierRunning = false;
  bool fanRunning = false;
  bool fanRunOnActive = false;
  unsigned long fanRunOnRemainingMs = 0;
  int updateCount = 0;

  if (!readBoolArg("enabled", enabled) ||
      !readFloatArg("temperatureC", temperatureC) ||
      !readBoolArg("hasTemperature", hasTemperature) ||
      !readBoolArg("sensorDisconnected", sensorDisconnected) ||
      !readBoolArg("peltierRunning", peltierRunning) ||
      !readBoolArg("fanRunning", fanRunning) ||
      !readBoolArg("fanRunOnActive", fanRunOnActive) ||
      !readUnsignedLongArg("fanRunOnRemainingMs", fanRunOnRemainingMs) ||
      !readIntArg("updateCount", updateCount)) {
    return false;
  }

  state.enabled = enabled;
  state.temperatureC = temperatureC;
  state.hasTemperature = hasTemperature;
  state.sensorDisconnected = sensorDisconnected;
  state.updateCount = updateCount < 0 ? 0 : updateCount;
  state.coolingState.peltierRunning = peltierRunning;
  state.coolingState.fanRunning = fanRunning;
  state.coolingState.fanRunOnActive = fanRunOnActive;
  state.coolingState.fanRunOnRemainingMs = fanRunOnRemainingMs;
  return true;
}

bool WebDashboard::readBoolArg(const char *name, bool &value)
{
  if (!server_.hasArg(name)) {
    return false;
  }

  const String text = server_.arg(name);
  if (text == "1" || text == "true" || text == "on") {
    value = true;
    return true;
  }
  if (text == "0" || text == "false" || text == "off") {
    value = false;
    return true;
  }
  return false;
}

bool WebDashboard::readStringArg(const char *name, String &value)
{
  if (!server_.hasArg(name)) {
    return false;
  }

  value = server_.arg(name);
  return true;
}

bool WebDashboard::readIntArg(const char *name, int &value)
{
  if (!server_.hasArg(name)) {
    return false;
  }

  const String text = server_.arg(name);
  if (text.length() == 0) {
    return false;
  }

  char *end = nullptr;
  const long parsed = strtol(text.c_str(), &end, 10);
  if (end == text.c_str() || *end != '\0') {
    return false;
  }
  value = static_cast<int>(parsed);
  return true;
}

bool WebDashboard::readFloatArg(const char *name, float &value)
{
  if (!server_.hasArg(name)) {
    return false;
  }

  const String text = server_.arg(name);
  if (text.length() == 0) {
    return false;
  }

  char *end = nullptr;
  value = strtof(text.c_str(), &end);
  return end != text.c_str() && *end == '\0';
}

bool WebDashboard::readUnsignedLongArg(const char *name,
                                       unsigned long &value)
{
  if (!server_.hasArg(name)) {
    return false;
  }

  const String text = server_.arg(name);
  if (text.length() == 0 || text[0] == '-') {
    return false;
  }

  char *end = nullptr;
  value = strtoul(text.c_str(), &end, 10);
  return end != text.c_str() && *end == '\0';
}

void WebDashboard::recordTargetChange(unsigned long uptimeMs)
{
  DashboardSnapshot snapshot = snapshot_;
  snapshot.uptimeMs = uptimeMs;
  snapshot.settings = settings_;
  recordTemperatureHistory(snapshot, true);
}

void WebDashboard::recordTemperatureHistory(const DashboardSnapshot &snapshot,
                                            bool forceTargetChange)
{
  const bool disconnected =
      !snapshot.hasTemperature || snapshot.sensorDisconnected;
  float storageTemperature = storedFilteredTemperatureC_;
  bool hasStorageTemperature = hasStoredFilteredTemperature_;

  if (!disconnected) {
    storageTemperature = hasStoredFilteredTemperature_
                             ? lowPassFilter(storedFilteredTemperatureC_,
                                             snapshot.temperatureC,
                                             0.18F)
                             : snapshot.temperatureC;
    storedFilteredTemperatureC_ = storageTemperature;
    hasStoredFilteredTemperature_ = true;
    hasStorageTemperature = true;
  }

  const int16_t temperatureCx10 =
      hasStorageTemperature ? temperatureToCx10(storageTemperature) : 0;
  const int16_t targetCx10 =
      temperatureToCx10(snapshot.settings.targetTemperatureC);
  const bool firstPoint = !hasTemperatureHistorySample_;
  const bool temperatureChanged =
      !disconnected && (!hasTemperatureHistorySample_ ||
                        std::abs(static_cast<int>(temperatureCx10) -
                                 static_cast<int>(lastStoredTemperatureCx10_)) >=
                            kTemperatureHistoryChangeCx10);
  const bool targetChanged =
      forceTargetChange ||
      !hasTemperatureHistorySample_ ||
      targetCx10 != lastStoredTargetCx10_;
  const bool keepaliveDue =
      hasTemperatureHistorySample_ &&
      snapshot.uptimeMs - lastTemperatureHistorySampleMs_ >=
          kTemperatureHistoryKeepaliveMs;
  const bool sensorChanged =
      !hasTemperatureHistorySample_ ||
      disconnected != lastStoredSensorDisconnected_;

  if (!firstPoint && !temperatureChanged && !targetChanged && !keepaliveDue &&
      !sensorChanged) {
    return;
  }

  TemperatureHistorySample sample;
  sample.uptimeMs = snapshot.uptimeMs;
  sample.temperatureCx10 = disconnected ? 0 : temperatureCx10;
  sample.targetCx10 = targetCx10;
  sample.flags = disconnected ? kTemperatureHistoryDisconnectedFlag : 0U;
  appendTemperatureHistorySample(sample);
}

void WebDashboard::appendTemperatureHistorySample(
    const TemperatureHistorySample &sample)
{
  size_t index = 0;
  if (temperatureHistoryCount_ < kTemperatureHistoryCapacity) {
    index = (temperatureHistoryStart_ + temperatureHistoryCount_) %
            kTemperatureHistoryCapacity;
    ++temperatureHistoryCount_;
  } else {
    index = temperatureHistoryStart_;
    temperatureHistoryStart_ =
        (temperatureHistoryStart_ + 1) % kTemperatureHistoryCapacity;
  }

  temperatureHistory_[index] = sample;
  lastTemperatureHistorySampleMs_ = sample.uptimeMs;
  lastStoredTemperatureCx10_ = sample.temperatureCx10;
  lastStoredTargetCx10_ = sample.targetCx10;
  lastStoredSensorDisconnected_ =
      (sample.flags & kTemperatureHistoryDisconnectedFlag) != 0;
  hasTemperatureHistorySample_ = true;
}

DashboardSnapshot WebDashboard::effectiveSnapshot() const
{
  if (!devState_.enabled) {
    return snapshot_;
  }

  DashboardSnapshot snapshot;
  snapshot.temperatureC = devState_.temperatureC;
  snapshot.hasTemperature = devState_.hasTemperature;
  snapshot.sensorDisconnected = devState_.sensorDisconnected;
  snapshot.updateCount = devState_.updateCount;
  snapshot.coolingState = devState_.coolingState;
  snapshot.settings = settings_;
  snapshot.uptimeMs = millis();
  return snapshot;
}

int16_t WebDashboard::temperatureToCx10(float temperatureC) const
{
  const float scaledTemperature = temperatureC * 10.0F;
  return static_cast<int16_t>(scaledTemperature >= 0.0F
                                  ? scaledTemperature + 0.5F
                                  : scaledTemperature - 0.5F);
}

float WebDashboard::lowPassFilter(float previous,
                                  float current,
                                  float alpha) const
{
  return previous + (current - previous) * alpha;
}
