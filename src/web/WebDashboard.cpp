#include "web/WebDashboard.h"

#include <WiFi.h>
#include <cstdlib>
#include "config/Config.h"
#include "debug/DebugLog.h"
#include "domain/TemperatureLogic.h"
#include "web/WebDashboardPage.h"

namespace {

constexpr unsigned long kNetworkScanCacheMs = 30000;
constexpr unsigned long kNetworkScanFailureCacheMs = 5000;
constexpr unsigned long kStationConnectPrepareMs = 250;

bool isSensorDisconnected(float temperatureC)
{
  return classifyTemperature(temperatureC) == TemperatureStatus::Disconnected;
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
  server_.on("/api/dev", HTTP_GET, [this]() { handleGetDevState(); });
  server_.on("/api/dev", HTTP_POST, [this]() { handleSaveDevState(); });
  server_.on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
  server_.on("/api/settings", HTTP_POST, [this]() { handleSaveSettings(); });
  server_.on("/api/networks", HTTP_GET, [this]() { handleNetworks(); });
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
}

void WebDashboard::setSettings(const AppSettings &settings)
{
  const String previousSsid = settings_.stationSsid;
  const String previousPassword = settings_.stationPassword;
  settings_ = sanitizedSettings(settings);
  persistedSettings_ = settings_;
  snapshot_.settings = settings_;
  if (networkStarted_ &&
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

  const String previousSsid = settings_.stationSsid;
  const String previousPassword = settings_.stationPassword;
  settings_ = sanitizedSettings(settings);
  snapshot_.settings = settings_;
  const bool stationChanged = settings_.stationSsid != previousSsid ||
                              settings_.stationPassword != previousPassword;
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
    if (stationChanged) {
      connectStation();
    }
  }
  server_.send(200, "application/json", settingsJson());
}

void WebDashboard::handleNetworks()
{
  server_.send(200, "application/json", networksJson());
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
  json += "}";
  return json;
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
  json += "}";
  return json;
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
  if (settings_.stationPassword.length() == 0) {
    WiFi.begin(settings_.stationSsid.c_str());
  } else {
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
  String json;
  json.reserve(180 + (networkCount > 0 ? networkCount * 96 : 0));
  json += "{\"ok\":true";
  json += ",\"scanResult\":";
  json += networkCount;
  json += ",\"scanStatus\":\"complete\"";
  json += ",\"networks\":[";
  if (networkCount > 0) {
    for (int i = 0; i < networkCount; ++i) {
      if (i > 0) {
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
    }
  }
  json += "],\"count\":";
  json += networkCount > 0 ? networkCount : 0;
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
  bool clearStationPassword = false;

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
  if (server_.hasArg("clearStationPassword") &&
      !readBoolArg("clearStationPassword", clearStationPassword)) {
    return false;
  }

  settings.targetTemperatureC = targetC;
  settings.hysteresisC = hysteresisC;
  settings.measurementIntervalMs = measurementIntervalMs;
  settings.fanRunOnMs = fanRunOnMs;
  settings.stationSsid = stationSsid;
  settings.stationPassword = settings_.stationPassword;
  const bool stationSsidChanged = stationSsid != settings_.stationSsid;
  if (clearStationPassword || stationSsid.length() == 0 ||
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
