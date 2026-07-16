#include "StatusFirmware.h"

#include <Arduino.h>

#include "web/StatusWebPage.h"

namespace {

constexpr const char *kAccessPointSsid = "StatusDevice";
constexpr const char *kAccessPointPassword = "status123";
constexpr const char *kOtaHostname = "status-device";

NetworkManagerOptions networkOptions()
{
  NetworkManagerOptions options;
  options.accessPointSsid = kAccessPointSsid;
  options.accessPointPassword = kAccessPointPassword;
  options.stationConnectTimeoutMs = 15000U;
  options.stationReconnectIntervalMs = 30000U;
  return options;
}

OtaManagerConfig otaOptions()
{
  OtaManagerConfig options;
  options.enabled = true;
  options.hostname = kOtaHostname;
  options.serialBaud = 115200U;
  return options;
}

BlinkFeatureOptions blinkOptions()
{
  BlinkFeatureOptions options;
  options.ledPin = LED_BUILTIN;
  options.otaHostname = kOtaHostname;
  options.otaPasswordSet = false;
  return options;
}

} // namespace

StatusFirmware::StatusFirmware()
    : network_(networkOptions()),
      web_(80U, webPageForPath),
      ota_(otaOptions()),
      firmwareUpdate_(web_.server(), system_),
      context_{clock_, settings_, network_, telemetry_},
      blink_(blinkOptions())
{
}

void StatusFirmware::begin()
{
  blink_.begin(context_);
  network_.begin();
  blink_.registerApi(web_.router());
  firmwareUpdate_.registerApi(web_.router());
  blink_.setOtaStatus(ota_.status());
  web_.begin();
}

void StatusFirmware::update()
{
  const uint32_t nowMs = clock_.nowMs();
  network_.update();
  web_.update();
  ota_.beginIfReady();
  ota_.update();
  blink_.setOtaStatus(ota_.status());
  blink_.tick(nowMs);
  firmwareUpdate_.update(nowMs);
}
