#include "app/CoolingApp.h"

#include "config/Config.h"
#include "debug/DebugLog.h"

namespace {

void beginSerial()
{
  DEBUG_SERIAL_BEGIN(Config::Debug::SerialBaud);
  DEBUG_PRINTLN("Cooling controller started");
}

void showStartupProgress(void *context,
                         const char *message,
                         unsigned long elapsedMs,
                         unsigned long totalMs)
{
  static_cast<OledView *>(context)->showBootStatus(message, elapsedMs, totalMs);
}

} // namespace

void CoolingApp::begin()
{
  beginSerial();
  view_.begin();
  view_.showBootStatus("Starting...");
  settingsStore_.begin();
  view_.showBootStatus("Loading settings");
  applySettings(settingsStore_.load());
  relays_.begin();
  view_.showBootStatus("Starting network");
  dashboard_.setSettings(settings_);
  dashboard_.setStartupProgressHandler(showStartupProgress, &view_);
  const IPAddress ip = dashboard_.begin();
  dashboard_.setStartupProgressHandler(nullptr, nullptr);
  view_.setIpAddress(ip);
  view_.showStartup();
  probe_.begin();
  relays_.startSelfTest(millis());
}

void CoolingApp::update()
{
  const unsigned long now = millis();
  dashboard_.setOtaStatus(ota_.status());
  dashboard_.update();
  ota_.beginIfReady();
  ota_.update();
  dashboard_.setOtaStatus(ota_.status());

  AppSettings pendingSettings;
  if (dashboard_.takePendingSettings(pendingSettings)) {
    applySettings(pendingSettings);
    settingsStore_.save(settings_);
  }

  relays_.updateSelfTest(now);

  if (relays_.selfTestRunning()) {
    return;
  }

  float temperatureC = 0.0F;
  if (probe_.update(now, temperatureC)) {
    updateTemperature(now, temperatureC);
  }
}

void CoolingApp::applySettings(const AppSettings &settings)
{
  settings_ = sanitizedSettings(settings);
  controller_.setSettings(settings_);
  probe_.setSettings(settings_);
}

void CoolingApp::updateTemperature(unsigned long nowMs, float temperatureC)
{
  const CoolingState &coolingState = controller_.update(temperatureC, nowMs);
  relays_.setOutputs(coolingState.peltierRunning, coolingState.fanRunning);
  probe_.recordUpdate();

  DashboardSnapshot snapshot;
  snapshot.temperatureC = temperatureC;
  snapshot.hasTemperature = true;
  snapshot.updateCount = probe_.updateCount();
  snapshot.coolingState = coolingState;
  snapshot.settings = settings_;
  snapshot.uptimeMs = nowMs;
  dashboard_.setSnapshot(snapshot);

  view_.showTemperature(temperatureC,
                        probe_.updateCount(),
                        settings_,
                        coolingState);
}
