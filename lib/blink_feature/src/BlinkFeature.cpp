#include "blink_feature/BlinkFeature.h"

#include <cstdlib>

#include "config/NetworkConfig.h"
#include "web/JsonHelpers.h"

namespace {

constexpr const char *kSettingsNamespace = "status-device";
constexpr const char *kDeviceNameKey = "deviceName";
constexpr const char *kBlinkIntervalKey = "blinkMs";
constexpr const char *kStationSsidKey = "staSsid";
constexpr const char *kStationPasswordKey = "staPass";
constexpr size_t kMaxDeviceNameLength = 32U;
constexpr uint32_t kMinBlinkIntervalMs = 100U;
constexpr uint32_t kMaxBlinkIntervalMs = 60000U;

String sanitizeDeviceName(const String &value)
{
  String sanitized = value;
  sanitized.trim();
  if (sanitized.length() > kMaxDeviceNameLength) {
    sanitized.remove(kMaxDeviceNameLength);
  }
  return sanitized.length() > 0 ? sanitized : String("Status Device");
}

uint32_t sanitizeBlinkInterval(uint32_t value)
{
  if (value < kMinBlinkIntervalMs) {
    return kMinBlinkIntervalMs;
  }
  if (value > kMaxBlinkIntervalMs) {
    return kMaxBlinkIntervalMs;
  }
  return value;
}

bool parseUnsigned(const String &text, uint32_t &value)
{
  if (text.length() == 0 || text[0] == '-') {
    return false;
  }
  char *end = nullptr;
  const unsigned long parsed = std::strtoul(text.c_str(), &end, 10);
  if (end == text.c_str() || *end != '\0') {
    return false;
  }
  value = static_cast<uint32_t>(parsed);
  return true;
}

bool parseBool(const String &text, bool &value)
{
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

} // namespace

BlinkFeature::BlinkFeature(const BlinkFeatureOptions &options)
    : options_(options)
{
}

void BlinkFeature::begin(AppContext &context)
{
  context_ = &context;
  context_->settings.begin(kSettingsNamespace);
  loadSettings();
  pinMode(options_.ledPin, OUTPUT);
  digitalWrite(options_.ledPin, LOW);
  lastToggleMs_ = context_->clock.nowMs();
  context_->telemetry.registerSource("blink");
}

void BlinkFeature::tick(uint32_t nowMs)
{
  if (static_cast<uint32_t>(nowMs - lastToggleMs_) < blinkIntervalMs_) {
    return;
  }
  lastToggleMs_ = nowMs;
  ledOn_ = !ledOn_;
  digitalWrite(options_.ledPin, ledOn_ ? HIGH : LOW);
  ++toggleCount_;
}

void BlinkFeature::registerApi(ApiRouter &router)
{
  server_ = &router.server();
  router.get("/api/blink/status", [this]() { handleStatus(); });
  router.get("/api/blink/settings", [this]() { handleGetSettings(); });
  router.post("/api/blink/settings", [this]() { handleSaveSettings(); });
  router.get("/api/networks", [this]() { handleNetworks(); });
}

void BlinkFeature::setOtaStatus(const OtaStatus &status)
{
  otaStatus_ = status;
}

void BlinkFeature::handleStatus()
{
  server_->send(200, "application/json", statusJson());
}

void BlinkFeature::handleGetSettings()
{
  server_->send(200, "application/json", settingsJson());
}

void BlinkFeature::handleSaveSettings()
{
  String deviceName;
  uint32_t blinkIntervalMs = 0U;
  NetworkConfig network;
  if (!readSettings(deviceName, blinkIntervalMs, network)) {
    server_->send(400,
                  "application/json",
                  "{\"ok\":false,\"error\":\"invalid settings\"}");
    return;
  }

  deviceName_ = sanitizeDeviceName(deviceName);
  blinkIntervalMs_ = sanitizeBlinkInterval(blinkIntervalMs);
  context_->network.setConfig(sanitizeNetworkConfig(network));
  if (!saveSettings()) {
    server_->send(500,
                  "application/json",
                  "{\"ok\":false,\"error\":\"settings save failed\"}");
    return;
  }
  server_->send(200, "application/json", settingsJson());
}

void BlinkFeature::handleNetworks()
{
  server_->send(200,
                "application/json",
                context_->network.networksJson());
}

String BlinkFeature::statusJson() const
{
  String json;
  json.reserve(520);
  json += "{\"deviceName\":";
  json += JsonHelpers::escapedString(deviceName_);
  json += ",\"ledOn\":";
  json += JsonHelpers::boolText(ledOn_);
  json += ",\"toggleCount\":";
  json += toggleCount_;
  json += ",\"blinkIntervalMs\":";
  json += blinkIntervalMs_;
  json += ",\"uptimeMs\":";
  json += context_->clock.nowMs();
  appendNetworkJson(json);
  appendOtaJson(json);
  json += "}";
  return json;
}

String BlinkFeature::settingsJson() const
{
  String json;
  json.reserve(500);
  json += "{\"ok\":true,\"deviceName\":";
  json += JsonHelpers::escapedString(deviceName_);
  json += ",\"blinkIntervalMs\":";
  json += blinkIntervalMs_;
  appendNetworkJson(json);
  appendOtaJson(json);
  json += "}";
  return json;
}

void BlinkFeature::appendNetworkJson(String &json) const
{
  const NetworkStatus network = context_->network.status();
  json += ",\"accessPointSsid\":";
  json += JsonHelpers::escapedString(network.accessPointSsid);
  json += ",\"accessPointIp\":";
  json += JsonHelpers::escapedString(network.accessPointIp.toString());
  json += ",\"stationSsid\":";
  json += JsonHelpers::escapedString(network.stationSsid);
  json += ",\"stationPasswordSet\":";
  json += JsonHelpers::boolText(
      context_->network.config().stationPassword.length() > 0);
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
}

void BlinkFeature::appendOtaJson(String &json) const
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
  json += JsonHelpers::escapedString(options_.otaHostname);
  json += ",\"otaPasswordSet\":";
  json += JsonHelpers::boolText(options_.otaPasswordSet);
  json += ",\"otaLastError\":";
  json += JsonHelpers::escapedString(otaStatus_.lastError);
}

bool BlinkFeature::readSettings(String &deviceName,
                                uint32_t &blinkIntervalMs,
                                NetworkConfig &network) const
{
  if (!server_->hasArg("deviceName") ||
      !server_->hasArg("blinkIntervalMs") ||
      !server_->hasArg("stationSsid")) {
    return false;
  }
  deviceName = server_->arg("deviceName");
  if (!parseUnsigned(server_->arg("blinkIntervalMs"), blinkIntervalMs)) {
    return false;
  }

  String stationSsid = server_->arg("stationSsid");
  String stationPassword = server_->hasArg("stationPassword")
                               ? server_->arg("stationPassword")
                               : String("");
  bool forgetNetwork = false;
  if (server_->hasArg("forgetStationNetwork") &&
      !parseBool(server_->arg("forgetStationNetwork"), forgetNetwork)) {
    return false;
  }

  network.stationPassword = context_->network.config().stationPassword;
  const bool stationChanged =
      stationSsid != context_->network.config().stationSsid;
  if (forgetNetwork) {
    stationSsid = "";
    stationPassword = "";
  }
  network.stationSsid = stationSsid;
  if (forgetNetwork || stationSsid.length() == 0 ||
      (stationChanged && stationPassword.length() == 0)) {
    network.stationPassword = "";
  } else if (stationPassword.length() > 0) {
    network.stationPassword = stationPassword;
  }
  return true;
}

bool BlinkFeature::loadSettings()
{
  char deviceName[kMaxDeviceNameLength + 1U] = {};
  char stationSsid[NetworkConfigLimits::MaxStationSsidLength + 1U] = {};
  char stationPassword[NetworkConfigLimits::MaxStationPasswordLength + 1U] = {};
  context_->settings.getString(kDeviceNameKey,
                               deviceName_.c_str(),
                               deviceName,
                               sizeof(deviceName));
  blinkIntervalMs_ = sanitizeBlinkInterval(context_->settings.getUInt32(
      kBlinkIntervalKey, blinkIntervalMs_));
  context_->settings.getString(kStationSsidKey,
                               "",
                               stationSsid,
                               sizeof(stationSsid));
  context_->settings.getString(kStationPasswordKey,
                               "",
                               stationPassword,
                               sizeof(stationPassword));
  deviceName_ = sanitizeDeviceName(deviceName);
  NetworkConfig network;
  network.stationSsid = stationSsid;
  network.stationPassword = stationPassword;
  context_->network.setConfig(sanitizeNetworkConfig(network));
  return true;
}

bool BlinkFeature::saveSettings() const
{
  const NetworkConfig network =
      sanitizeNetworkConfig(context_->network.config());
  bool ok = true;
  ok = context_->settings.putString(kDeviceNameKey, deviceName_.c_str()) && ok;
  ok = context_->settings.putUInt32(kBlinkIntervalKey, blinkIntervalMs_) && ok;
  ok = context_->settings.putString(kStationSsidKey,
                                    network.stationSsid.c_str()) && ok;
  ok = context_->settings.putString(kStationPasswordKey,
                                    network.stationPassword.c_str()) && ok;
  return ok;
}
