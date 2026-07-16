#include "product/CoolingFirmware.h"

#include "config/Config.h"
#include "web/WebDashboardPage.h"

namespace {

NetworkManagerOptions networkOptions()
{
  NetworkManagerOptions options;
  options.accessPointSsid = Config::Network::AccessPointSsid;
  options.accessPointPassword = Config::Network::AccessPointPassword;
  options.stationConnectTimeoutMs = Config::Network::StationConnectTimeoutMs;
  options.stationReconnectIntervalMs =
      Config::Network::StationReconnectIntervalMs;
  return options;
}

OtaManagerConfig otaOptions()
{
  OtaManagerConfig options;
  options.enabled = Config::Ota::Enabled;
  options.hostname = Config::Ota::Hostname;
  options.password = Config::Ota::Password;
  options.passwordHash = Config::Ota::PasswordHash;
  options.serialBaud = Config::Debug::SerialBaud;
  return options;
}

} // namespace

CoolingFirmware::CoolingFirmware()
    : network_(networkOptions()),
      web_(Config::Network::WebServerPort, webPageForPath),
      ota_(otaOptions()),
      firmwareUpdate_(web_.server(), system_),
      context_{clock_, settings_, network_, telemetry_}
{
}

void CoolingFirmware::begin()
{
  cooling_.begin(context_);
  const IPAddress accessPointIp = network_.begin();
  cooling_.onNetworkStarted(accessPointIp);
  cooling_.registerApi(web_.router());
  firmwareUpdate_.registerApi(web_.router());
  cooling_.setOtaStatus(ota_.status());
  web_.begin();
}

void CoolingFirmware::update()
{
  const uint32_t nowMs = clock_.nowMs();
  cooling_.setOtaStatus(ota_.status());
  network_.update();
  web_.update();
  ota_.beginIfReady();
  ota_.update();
  cooling_.setOtaStatus(ota_.status());
  cooling_.tick(nowMs);
  firmwareUpdate_.update(nowMs);
}
