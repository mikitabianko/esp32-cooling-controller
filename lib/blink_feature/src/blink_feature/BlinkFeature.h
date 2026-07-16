#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "app/AppContext.h"
#include "ota/OtaManager.h"
#include "web/ApiRouter.h"

struct BlinkFeatureOptions {
  uint8_t ledPin = LED_BUILTIN;
  const char *otaHostname = "status-device";
  bool otaPasswordSet = false;
};

class BlinkFeature {
public:
  explicit BlinkFeature(const BlinkFeatureOptions &options);

  void begin(AppContext &context);
  void tick(uint32_t nowMs);
  void registerApi(ApiRouter &router);
  void setOtaStatus(const OtaStatus &status);

private:
  void handleStatus();
  void handleGetSettings();
  void handleSaveSettings();
  void handleNetworks();
  String statusJson() const;
  String settingsJson() const;
  void appendNetworkJson(String &json) const;
  void appendOtaJson(String &json) const;
  bool readSettings(String &deviceName,
                    uint32_t &blinkIntervalMs,
                    NetworkConfig &network) const;
  bool loadSettings();
  bool saveSettings() const;

  BlinkFeatureOptions options_;
  AppContext *context_ = nullptr;
  WebServer *server_ = nullptr;
  String deviceName_ = "Status Device";
  uint32_t blinkIntervalMs_ = 1000U;
  uint32_t lastToggleMs_ = 0U;
  uint32_t toggleCount_ = 0U;
  bool ledOn_ = false;
  OtaStatus otaStatus_;
};
