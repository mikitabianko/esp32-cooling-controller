#include "web/CoolingApi.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "config/Config.h"
#include "web/JsonHelpers.h"

void CoolingApi::begin(NetworkManager &network)
{
  network_ = &network;
}

void CoolingApi::registerApi(ApiRouter &router)
{
  server_ = &router.server();
  router.any("/api/status", [this]() { handleStatus(); });
  router.get("/api/history", [this]() { handleHistory(); });
  router.get("/api/history.csv", [this]() { handleHistoryCsv(); });
  router.get("/api/dev", [this]() { handleGetDevState(); });
  router.post("/api/dev", [this]() { handleSaveDevState(); });
  router.get("/api/settings", [this]() { handleGetSettings(); });
  router.post("/api/settings", [this]() { handleSaveSettings(); });
  router.get("/api/networks", [this]() { handleNetworks(); });
}

void CoolingApi::update()
{
  const NetworkCredentialResult result = network_->takeCredentialResult();
  if (result == NetworkCredentialResult::Confirmed) {
    queueConfigSave(coolingConfig_, network_->config());
  }
}

void CoolingApi::setSnapshot(const DashboardSnapshot &snapshot)
{
  snapshot_ = snapshot;
  snapshot_.coolingConfig = coolingConfig_;
  if (snapshot_.hasTemperature && snapshot_.sensorAvailable) {
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

void CoolingApi::setConfig(const CoolingConfig &cooling,
                           const NetworkConfig &network)
{
  coolingConfig_ = sanitizeCoolingConfig(cooling);
  snapshot_.coolingConfig = coolingConfig_;
  network_->setConfig(network);
}

void CoolingApi::setOtaStatus(const OtaStatus &status)
{
  otaStatus_ = status;
}

bool CoolingApi::takePendingConfig(CoolingConfig &cooling,
                                   NetworkConfig &network)
{
  if (!hasPendingConfig_) {
    return false;
  }
  cooling = pendingCoolingConfig_;
  network = pendingNetworkConfig_;
  hasPendingConfig_ = false;
  return true;
}

void CoolingApi::handleStatus()
{
  server_->send(200, "application/json", statusJson());
}

void CoolingApi::handleHistory()
{
  server_->send(200, "application/json", historyJson());
}

void CoolingApi::handleHistoryCsv()
{
  server_->send(200, "text/csv; charset=utf-8", historyCsv());
}

void CoolingApi::handleGetDevState()
{
  server_->send(200, "application/json", devJson());
}

void CoolingApi::handleSaveDevState()
{
  DevDashboardState state;
  if (!readDevArgs(state)) {
    server_->send(400,
                 "application/json",
                 "{\"ok\":false,\"error\":\"invalid dev state\"}");
    return;
  }
  devState_ = state;
  server_->send(200, "application/json", devJson());
}

void CoolingApi::handleGetSettings()
{
  server_->send(200, "application/json", settingsJson());
}

void CoolingApi::handleSaveSettings()
{
  CoolingConfig cooling;
  NetworkConfig network;
  if (!readConfigArgs(cooling, network)) {
    server_->send(400,
                 "application/json",
                 "{\"ok\":false,\"error\":\"invalid settings\"}");
    return;
  }

  const CoolingConfig previousCooling = coolingConfig_;
  const NetworkConfig previousNetwork = network_->config();
  coolingConfig_ = sanitizeCoolingConfig(cooling);
  network = sanitizeNetworkConfig(network);
  snapshot_.coolingConfig = coolingConfig_;
  const bool targetChanged =
      std::fabs(coolingConfig_.targetTemperatureC -
                previousCooling.targetTemperatureC) >= 0.05F;
  const bool stationChanged =
      network.stationSsid != previousNetwork.stationSsid ||
      network.stationPassword != previousNetwork.stationPassword;
  const bool stationNeedsTrial =
      stationChanged && network.stationSsid.length() > 0;

  if (stationNeedsTrial) {
    queueConfigSave(coolingConfig_, network_->persistedConfig());
    network_->setConfig(network, true);
  } else {
    network_->setConfig(network);
    queueConfigSave(coolingConfig_, network);
  }
  if (targetChanged) {
    recordTargetChange(millis());
  }
  server_->send(200, "application/json", settingsJson());
}

void CoolingApi::handleNetworks()
{
  server_->send(200, "application/json", network_->networksJson());
}

String CoolingApi::statusJson() const
{
  const DashboardSnapshot snapshot = effectiveSnapshot();
  const NetworkStatus network = network_->status();
  String json;
  json.reserve(560);
  json += "{";
  json += "\"temperatureC\":";
  json += snapshot.temperatureC;
  json += ",\"filteredTemperatureC\":";
  json += snapshot.temperatureC;
  json += ",\"hasTemperature\":";
  json += JsonHelpers::boolText(snapshot.hasTemperature);
  json += ",\"sensorDisconnected\":";
  json += JsonHelpers::boolText(snapshot.hasTemperature &&
                                !snapshot.sensorAvailable);
  json += ",\"updateCount\":";
  json += snapshot.updateCount;
  json += ",\"peltierRunning\":";
  json += JsonHelpers::boolText(snapshot.coolingState.peltierEnabled);
  json += ",\"fanRunning\":";
  json += JsonHelpers::boolText(snapshot.coolingState.fanEnabled);
  json += ",\"fanRunOnActive\":";
  json += JsonHelpers::boolText(snapshot.coolingState.fanRunOnActive);
  json += ",\"fanRunOnRemainingMs\":";
  json += snapshot.coolingState.fanRunOnRemainingMs;
  json += ",\"targetC\":";
  json += coolingConfig_.targetTemperatureC;
  json += ",\"hysteresisC\":";
  json += coolingConfig_.hysteresisC;
  json += ",\"measurementIntervalMs\":";
  json += coolingConfig_.measurementIntervalMs;
  json += ",\"fanRunOnMs\":";
  json += coolingConfig_.fanRunOnMs;
  json += ",\"uptimeMs\":";
  json += snapshot.uptimeMs;
  json += ",\"devMode\":";
  json += JsonHelpers::boolText(devState_.enabled);
  json += ",\"accessPointSsid\":";
  json += JsonHelpers::escapedString(Config::Network::AccessPointSsid);
  json += ",\"accessPointIp\":";
  json += JsonHelpers::escapedString(network.accessPointIp.toString());
  json += ",\"stationSsid\":";
  json += JsonHelpers::escapedString(network.stationSsid);
  json += ",\"stationConnected\":";
  json += JsonHelpers::boolText(network.stationConnected);
  json += ",\"stationStatus\":";
  json += JsonHelpers::escapedString(network.stationState);
  json += ",\"stationLastFailure\":";
  json += JsonHelpers::escapedString(network.stationLastFailure);
  json += ",\"stationIp\":";
  json += JsonHelpers::escapedString(
      network.stationConnected ? network.stationIp.toString() : String(""));
  json += ",\"stationRssi\":";
  json += network.stationRssi;
  appendOtaJson(json);
  json += "}";
  return json;
}

String CoolingApi::historyJson() const
{
  String json;
  json.reserve(430 + temperatureHistory_.size() * 42U);
  json += "{";
  json += "\"ok\":true";
  json += ",\"capacity\":";
  json += temperatureHistory_.capacity();
  json += ",\"keepaliveMs\":";
  json += kTemperatureHistoryKeepaliveMs;
  json += ",\"changeThresholdCx10\":";
  json += kTemperatureHistoryChangeCx10;
  json += ",\"disconnectedFlag\":";
  json += kTemperatureHistoryDisconnectedFlag;
  json += ",\"peltierFlag\":";
  json += kTemperatureHistoryPeltierFlag;
  json += ",\"fanFlag\":";
  json += kTemperatureHistoryFanFlag;
  json += ",\"fanRunOnFlag\":";
  json += kTemperatureHistoryFanRunOnFlag;
  json += ",\"hysteresisC\":";
  json += coolingConfig_.hysteresisC;
  json += ",\"fields\":[\"uptimeMs\",\"temperatureCx10\",\"targetCx10\",\"flags\",\"sensorValid\",\"sensorDisconnected\",\"hysteresisCx10\",\"peltierRunning\",\"fanRunning\",\"fanRunOnActive\"]";
  json += ",\"series\":[{\"id\":\"probe1\",\"label\":\"Filtered temperature\",\"unit\":\"C\",\"points\":[";
  for (size_t i = 0; i < temperatureHistory_.size(); ++i) {
    if (i > 0) {
      json += ",";
    }
    const TemperatureHistorySample &sample = temperatureHistory_.at(i);
    const bool disconnected =
        (sample.flags & kTemperatureHistoryDisconnectedFlag) != 0;
    const bool peltierRunning =
        (sample.flags & kTemperatureHistoryPeltierFlag) != 0;
    const bool fanRunning =
        (sample.flags & kTemperatureHistoryFanFlag) != 0;
    const bool fanRunOnActive =
        (sample.flags & kTemperatureHistoryFanRunOnFlag) != 0;
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
    json += ",";
    json += temperatureToCx10(coolingConfig_.hysteresisC);
    json += ",";
    json += peltierRunning ? 1 : 0;
    json += ",";
    json += fanRunning ? 1 : 0;
    json += ",";
    json += fanRunOnActive ? 1 : 0;
    json += "]";
  }
  json += "]}]}";
  return json;
}

String CoolingApi::historyCsv() const
{
  String csv;
  csv.reserve(96 + temperatureHistory_.size() * 44U);
  csv += "timestamp_ms,filtered_temperature_c,target_temperature_c,hysteresis_c,peltier_running,fan_running,fan_runon_active,sensor_status,sensor_valid,sensor_disconnected,flags\n";
  for (size_t i = 0; i < temperatureHistory_.size(); ++i) {
    const TemperatureHistorySample &sample = temperatureHistory_.at(i);
    const bool disconnected =
        (sample.flags & kTemperatureHistoryDisconnectedFlag) != 0;
    const bool peltierRunning =
        (sample.flags & kTemperatureHistoryPeltierFlag) != 0;
    const bool fanRunning =
        (sample.flags & kTemperatureHistoryFanFlag) != 0;
    const bool fanRunOnActive =
        (sample.flags & kTemperatureHistoryFanRunOnFlag) != 0;
    csv += sample.uptimeMs;
    csv += ",";
    if (!disconnected) {
      csv += String(static_cast<float>(sample.temperatureCx10) / 10.0F, 1);
    }
    csv += ",";
    csv += String(static_cast<float>(sample.targetCx10) / 10.0F, 1);
    csv += ",";
    csv += String(coolingConfig_.hysteresisC, 1);
    csv += ",";
    csv += peltierRunning ? "1" : "0";
    csv += ",";
    csv += fanRunning ? "1" : "0";
    csv += ",";
    csv += fanRunOnActive ? "1" : "0";
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

String CoolingApi::settingsJson() const
{
  const NetworkStatus network = network_->status();
  String json;
  json.reserve(280);
  json += "{\"ok\":true";
  json += ",\"targetC\":";
  json += coolingConfig_.targetTemperatureC;
  json += ",\"hysteresisC\":";
  json += coolingConfig_.hysteresisC;
  json += ",\"measurementIntervalMs\":";
  json += coolingConfig_.measurementIntervalMs;
  json += ",\"fanRunOnMs\":";
  json += coolingConfig_.fanRunOnMs;
  json += ",\"accessPointSsid\":";
  json += JsonHelpers::escapedString(Config::Network::AccessPointSsid);
  json += ",\"accessPointIp\":";
  json += JsonHelpers::escapedString(network.accessPointIp.toString());
  json += ",\"stationSsid\":";
  json += JsonHelpers::escapedString(network.stationSsid);
  json += ",\"stationPasswordSet\":";
  json += JsonHelpers::boolText(network_->config().stationPassword.length() > 0);
  json += ",\"stationConnected\":";
  json += JsonHelpers::boolText(network.stationConnected);
  json += ",\"stationStatus\":";
  json += JsonHelpers::escapedString(network.stationState);
  json += ",\"stationLastFailure\":";
  json += JsonHelpers::escapedString(network.stationLastFailure);
  json += ",\"stationIp\":";
  json += JsonHelpers::escapedString(
      network.stationConnected ? network.stationIp.toString() : String(""));
  json += ",\"stationRssi\":";
  json += network.stationRssi;
  appendOtaJson(json);
  json += "}";
  return json;
}

String CoolingApi::devJson() const
{
  String json;
  json.reserve(260);
  json += "{\"ok\":true";
  json += ",\"enabled\":";
  json += JsonHelpers::boolText(devState_.enabled);
  json += ",\"temperatureC\":";
  json += devState_.temperatureC;
  json += ",\"hasTemperature\":";
  json += JsonHelpers::boolText(devState_.hasTemperature);
  json += ",\"sensorDisconnected\":";
  json += JsonHelpers::boolText(devState_.sensorDisconnected);
  json += ",\"updateCount\":";
  json += devState_.updateCount;
  json += ",\"peltierRunning\":";
  json += JsonHelpers::boolText(devState_.coolingState.peltierEnabled);
  json += ",\"fanRunning\":";
  json += JsonHelpers::boolText(devState_.coolingState.fanEnabled);
  json += ",\"fanRunOnActive\":";
  json += JsonHelpers::boolText(devState_.coolingState.fanRunOnActive);
  json += ",\"fanRunOnRemainingMs\":";
  json += devState_.coolingState.fanRunOnRemainingMs;
  json += "}";
  return json;
}

void CoolingApi::appendOtaJson(String &json) const
{
  json += ",\"otaEnabled\":";
  json += JsonHelpers::boolText(otaStatus_.enabled);
  json += ",\"otaStarted\":";
  json += JsonHelpers::boolText(otaStatus_.started);
  json += ",\"otaUpdating\":";
  json += JsonHelpers::boolText(otaStatus_.updating);
  json += ",\"otaProgress\":";
  json += otaStatus_.progressPercent;
  json += ",\"otaStatus\":";
  json += JsonHelpers::escapedString(otaStatus_.state);
  json += ",\"otaHostname\":";
  json += JsonHelpers::escapedString(Config::Ota::Hostname);
  json += ",\"otaPasswordSet\":";
  json += JsonHelpers::boolText(std::strlen(Config::Ota::Password) > 0 ||
                                std::strlen(Config::Ota::PasswordHash) > 0);
  json += ",\"otaLastError\":";
  json += JsonHelpers::escapedString(otaStatus_.lastError);
}

void CoolingApi::queueConfigSave(const CoolingConfig &cooling,
                                 const NetworkConfig &network)
{
  pendingCoolingConfig_ = sanitizeCoolingConfig(cooling);
  pendingNetworkConfig_ = sanitizeNetworkConfig(network);
  hasPendingConfig_ = true;
}

bool CoolingApi::readConfigArgs(CoolingConfig &cooling,
                                NetworkConfig &network)
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
  if (server_->hasArg("stationPassword") &&
      !readStringArg("stationPassword", stationPassword)) {
    return false;
  }
  if (server_->hasArg("forgetStationNetwork") &&
      !readBoolArg("forgetStationNetwork", forgetStationNetwork)) {
    return false;
  }
  if (!forgetStationNetwork && server_->hasArg("clearStationPassword") &&
      !readBoolArg("clearStationPassword", forgetStationNetwork)) {
    return false;
  }

  cooling.targetTemperatureC = targetC;
  cooling.hysteresisC = hysteresisC;
  cooling.measurementIntervalMs = measurementIntervalMs;
  cooling.fanRunOnMs = fanRunOnMs;
  network.stationPassword = network_->config().stationPassword;
  const bool stationSsidChanged =
      stationSsid != network_->config().stationSsid;
  if (forgetStationNetwork) {
    stationSsid = "";
    stationPassword = "";
  }
  network.stationSsid = stationSsid;
  if (forgetStationNetwork || stationSsid.length() == 0 ||
      (stationSsidChanged && stationPassword.length() == 0)) {
    network.stationPassword = "";
  } else if (stationPassword.length() > 0) {
    network.stationPassword = stationPassword;
  }
  cooling = sanitizeCoolingConfig(cooling);
  network = sanitizeNetworkConfig(network);
  return true;
}

bool CoolingApi::readDevArgs(DevDashboardState &state)
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
  state.coolingState.peltierEnabled = peltierRunning;
  state.coolingState.fanEnabled = fanRunning;
  state.coolingState.fanRunOnActive = fanRunOnActive;
  state.coolingState.fanRunOnRemainingMs = fanRunOnRemainingMs;
  return true;
}

bool CoolingApi::readBoolArg(const char *name, bool &value)
{
  if (!server_->hasArg(name)) {
    return false;
  }
  const String text = server_->arg(name);
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

bool CoolingApi::readStringArg(const char *name, String &value)
{
  if (!server_->hasArg(name)) {
    return false;
  }
  value = server_->arg(name);
  return true;
}

bool CoolingApi::readIntArg(const char *name, int &value)
{
  if (!server_->hasArg(name)) {
    return false;
  }
  const String text = server_->arg(name);
  if (text.length() == 0) {
    return false;
  }
  char *end = nullptr;
  const long parsed = std::strtol(text.c_str(), &end, 10);
  if (end == text.c_str() || *end != '\0') {
    return false;
  }
  value = static_cast<int>(parsed);
  return true;
}

bool CoolingApi::readFloatArg(const char *name, float &value)
{
  if (!server_->hasArg(name)) {
    return false;
  }
  const String text = server_->arg(name);
  if (text.length() == 0) {
    return false;
  }
  char *end = nullptr;
  value = std::strtof(text.c_str(), &end);
  return end != text.c_str() && *end == '\0';
}

bool CoolingApi::readUnsignedLongArg(const char *name,
                                     unsigned long &value)
{
  if (!server_->hasArg(name)) {
    return false;
  }
  const String text = server_->arg(name);
  if (text.length() == 0 || text[0] == '-') {
    return false;
  }
  char *end = nullptr;
  value = std::strtoul(text.c_str(), &end, 10);
  return end != text.c_str() && *end == '\0';
}

void CoolingApi::recordTargetChange(uint32_t uptimeMs)
{
  DashboardSnapshot snapshot = snapshot_;
  snapshot.uptimeMs = uptimeMs;
  snapshot.coolingConfig = coolingConfig_;
  recordTemperatureHistory(snapshot, true);
}

void CoolingApi::recordTemperatureHistory(const DashboardSnapshot &snapshot,
                                          bool forceTargetChange)
{
  const bool disconnected =
      !snapshot.hasTemperature || !snapshot.sensorAvailable;
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
      temperatureToCx10(snapshot.coolingConfig.targetTemperatureC);
  const bool firstPoint = !hasTemperatureHistorySample_;
  const bool temperatureChanged =
      !disconnected && (!hasTemperatureHistorySample_ ||
                        std::abs(static_cast<int>(temperatureCx10) -
                                 static_cast<int>(lastStoredTemperatureCx10_)) >=
                            kTemperatureHistoryChangeCx10);
  const bool targetChanged = forceTargetChange ||
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
  if (snapshot.coolingState.peltierEnabled) {
    sample.flags |= kTemperatureHistoryPeltierFlag;
  }
  if (snapshot.coolingState.fanEnabled) {
    sample.flags |= kTemperatureHistoryFanFlag;
  }
  if (snapshot.coolingState.fanRunOnActive) {
    sample.flags |= kTemperatureHistoryFanRunOnFlag;
  }
  temperatureHistory_.push(sample);
  lastTemperatureHistorySampleMs_ = sample.uptimeMs;
  lastStoredTemperatureCx10_ = sample.temperatureCx10;
  lastStoredTargetCx10_ = sample.targetCx10;
  lastStoredSensorDisconnected_ = disconnected;
  hasTemperatureHistorySample_ = true;
}

DashboardSnapshot CoolingApi::effectiveSnapshot() const
{
  if (!devState_.enabled) {
    return snapshot_;
  }
  DashboardSnapshot snapshot;
  snapshot.temperatureC = devState_.temperatureC;
  snapshot.hasTemperature = devState_.hasTemperature;
  snapshot.sensorAvailable = !devState_.sensorDisconnected;
  snapshot.updateCount = devState_.updateCount;
  snapshot.coolingState = devState_.coolingState;
  snapshot.coolingConfig = coolingConfig_;
  snapshot.uptimeMs = millis();
  return snapshot;
}

int16_t CoolingApi::temperatureToCx10(float temperatureC) const
{
  const float scaledTemperature = temperatureC * 10.0F;
  return static_cast<int16_t>(scaledTemperature >= 0.0F
                                  ? scaledTemperature + 0.5F
                                  : scaledTemperature - 0.5F);
}

float CoolingApi::lowPassFilter(float previous,
                                float current,
                                float alpha) const
{
  return previous + (current - previous) * alpha;
}
