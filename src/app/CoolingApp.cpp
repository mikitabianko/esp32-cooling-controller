#include "app/CoolingApp.h"

#include "config/Config.h"

namespace {

void beginSerial()
{
  Serial.begin(9600);
  Serial.println("Cooling controller started");
}

} // namespace

void CoolingApp::begin()
{
  beginSerial();
  settingsStore_.begin();
  applySettings(settingsStore_.load());
  relays_.begin();
  view_.begin();
  dashboard_.setSettings(settings_);
  const IPAddress ip = dashboard_.begin();
  view_.setIpAddress(ip);
  view_.showStartup();
  probe_.begin();
  relays_.startSelfTest(millis());
}

void CoolingApp::update()
{
  const unsigned long now = millis();
  dashboard_.update();

  AppSettings pendingSettings;
  if (dashboard_.takePendingSettings(pendingSettings)) {
    applySettings(pendingSettings);
    settingsStore_.save(settings_);
    dashboard_.setSettings(settings_);
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
