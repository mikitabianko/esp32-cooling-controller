#include "cooling_feature/CoolingFeature.h"

#include "config/Config.h"
#include "debug/DebugLog.h"

namespace {

void beginSerial()
{
  DEBUG_SERIAL_BEGIN(Config::Debug::SerialBaud);
  DEBUG_PRINTLN("Cooling controller started");
}

} // namespace

void CoolingFeature::begin(AppContext &context)
{
  context_ = &context;
  beginSerial();
  view_.begin();
  view_.showBootStatus("Starting...");
  settingsStore_.begin(context.settings);
  view_.showBootStatus("Loading settings");
  applyConfig(settingsStore_.load());
  relays_.begin();
  view_.showBootStatus("Starting network");
  api_.begin(context.network);
  api_.setConfig(config_.cooling, config_.network);
  context.network.setStartupProgressHandler(showStartupProgress, this);
  context.telemetry.registerSource("cooling");
}

void CoolingFeature::tick(uint32_t nowMs)
{
  api_.update();

  CoolingConfig pendingCooling;
  NetworkConfig pendingNetwork;
  if (api_.takePendingConfig(pendingCooling, pendingNetwork)) {
    DeviceConfig pendingConfig;
    pendingConfig.cooling = pendingCooling;
    pendingConfig.network = pendingNetwork;
    applyConfig(pendingConfig);
    settingsStore_.save(config_);
  }

  relays_.updateSelfTest(nowMs);
  if (relays_.selfTestRunning()) {
    return;
  }

  TemperatureReading reading;
  if (probe_.update(nowMs, reading)) {
    updateTemperature(nowMs, reading);
  }
}

void CoolingFeature::registerApi(ApiRouter &router)
{
  api_.registerApi(router);
}

void CoolingFeature::onNetworkStarted(const IPAddress &accessPointIp)
{
  context_->network.setStartupProgressHandler(nullptr, nullptr);
  view_.setIpAddress(accessPointIp);
  view_.showStartup();
  probe_.begin();
  relays_.startSelfTest(context_->clock.nowMs());
}

void CoolingFeature::setOtaStatus(const OtaStatus &status)
{
  api_.setOtaStatus(status);
}

void CoolingFeature::showStartupProgress(void *feature,
                                         const char *message,
                                         unsigned long elapsedMs,
                                         unsigned long totalMs)
{
  static_cast<CoolingFeature *>(feature)->view_.showBootStatus(
      message, elapsedMs, totalMs);
}

void CoolingFeature::applyConfig(const DeviceConfig &config)
{
  config_ = sanitizeDeviceConfig(config);
  TemperatureProbeConfig probeConfig;
  probeConfig.measurementIntervalMs = config_.cooling.measurementIntervalMs;
  probe_.setConfig(probeConfig);
}

void CoolingFeature::updateTemperature(uint32_t nowMs,
                                       const TemperatureReading &reading)
{
  CoolingInputs inputs;
  inputs.temperatureC = reading.temperatureC;
  inputs.sensorAvailable = reading.sensorAvailable;
  const CoolingOutputs coolingState =
      controller_.step(inputs, config_.cooling, nowMs);
  relays_.setOutputs(coolingState.peltierEnabled, coolingState.fanEnabled);
  logCoolingState(coolingState);
  probe_.recordUpdate();

  DashboardSnapshot snapshot;
  snapshot.temperatureC = reading.temperatureC;
  snapshot.hasTemperature = true;
  snapshot.sensorAvailable = reading.sensorAvailable;
  snapshot.updateCount = probe_.updateCount();
  snapshot.coolingState = coolingState;
  snapshot.coolingConfig = config_.cooling;
  snapshot.uptimeMs = nowMs;
  api_.setSnapshot(snapshot);

  view_.showTemperature(reading.temperatureC,
                        reading.sensorAvailable,
                        probe_.updateCount(),
                        config_.cooling,
                        config_.network,
                        coolingState);
}

void CoolingFeature::logCoolingState(const CoolingOutputs &outputs) const
{
  DEBUG_PRINT("Peltier: ");
  DEBUG_PRINT(outputs.peltierEnabled ? "ON" : "OFF");
  DEBUG_PRINT(" | fan: ");
  DEBUG_PRINT(outputs.fanEnabled ? "ON" : "OFF");
  if (outputs.fanRunOnActive) {
    DEBUG_PRINT(" | run-on ");
    DEBUG_PRINT(outputs.fanRunOnRemainingMs / 1000U);
    DEBUG_PRINT(" s");
  }
  DEBUG_PRINT(" | target ");
  DEBUG_PRINT2(config_.cooling.targetTemperatureC, 1);
  DEBUG_PRINT(" C | hysteresis ");
  DEBUG_PRINT2(config_.cooling.hysteresisC, 1);
  DEBUG_PRINTLN(" C");
}
