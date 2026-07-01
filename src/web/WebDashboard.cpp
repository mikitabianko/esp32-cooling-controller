#include "web/WebDashboard.h"

#include <WiFi.h>
#include <cstdlib>
#include "config/Config.h"
#include "domain/TemperatureLogic.h"
#include "web/WebDashboardPage.h"

namespace {

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
  connectStation();

  server_.on("/", [this]() { handlePage(); });
  server_.on("/api/status", [this]() { handleStatus(); });
  server_.on("/api/dev", HTTP_GET, [this]() { handleGetDevState(); });
  server_.on("/api/dev", HTTP_POST, [this]() { handleSaveDevState(); });
  server_.on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
  server_.on("/api/settings", HTTP_POST, [this]() { handleSaveSettings(); });
  server_.onNotFound([this]() { handleNotFound(); });
  server_.begin();

  Serial.print("WiFi AP: ");
  Serial.println(Config::Network::AccessPointSsid);
  Serial.print("AP web server: http://");
  Serial.println(ipAddress_);
  if (settings_.stationSsid.length() > 0) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("STA connected to: ");
      Serial.println(settings_.stationSsid);
      Serial.print("STA web server: http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.print("STA connection failed: ");
      Serial.println(settings_.stationSsid);
    }
  } else {
    Serial.println("STA disabled: StationSsid is empty");
  }

  return ipAddress_;
}

void WebDashboard::update()
{
  maintainStationConnection();
  server_.handleClient();
}

void WebDashboard::setSnapshot(const DashboardSnapshot &snapshot)
{
  snapshot_ = snapshot;
  settings_ = snapshot_.settings;
  snapshot_.sensorDisconnected =
      snapshot_.hasTemperature && isSensorDisconnected(snapshot_.temperatureC);
}

void WebDashboard::setSettings(const AppSettings &settings)
{
  const String previousSsid = settings_.stationSsid;
  const String previousPassword = settings_.stationPassword;
  settings_ = sanitizedSettings(settings);
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
  pendingSettings_ = sanitizedSettings(settings);
  settings_ = pendingSettings_;
  snapshot_.settings = settings_;
  if (settings_.stationSsid != previousSsid ||
      settings_.stationPassword != previousPassword) {
    connectStation();
  }
  hasPendingSettings_ = true;
  server_.send(200, "application/json", settingsJson());
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
  json += ",\"stationIp\":";
  json += jsonString(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString()
                                                   : String(""));
  json += ",\"stationRssi\":";
  json += WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  json += "}";
  return json;
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
    return "disconnected";
  case WL_IDLE_STATUS:
    return "connecting";
  default:
    return "unknown";
  }
}

void WebDashboard::connectStation()
{
  if (!networkStarted_) {
    return;
  }

  WiFi.disconnect(false, false);
  if (settings_.stationSsid.length() == 0) {
    return;
  }

  lastStationConnectAttemptMs_ = millis();
  Serial.print("STA connecting to: ");
  Serial.println(settings_.stationSsid);
  WiFi.begin(settings_.stationSsid.c_str(), settings_.stationPassword.c_str());
  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < Config::Network::StationConnectTimeoutMs) {
    delay(100);
  }
  Serial.print("STA status: ");
  Serial.println(stationStatusText());
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
  }
}

void WebDashboard::maintainStationConnection()
{
  if (!networkStarted_ || settings_.stationSsid.length() == 0 ||
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
  if (clearStationPassword || stationSsid.length() == 0) {
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
