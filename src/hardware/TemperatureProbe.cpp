#include "hardware/TemperatureProbe.h"

#include <Arduino.h>
#include "config/Config.h"
#include "debug/DebugLog.h"
#include "domain/TemperatureLogic.h"

TemperatureProbe::TemperatureProbe()
    : oneWire_(Config::Pins::OneWireBus),
      sensors_(&oneWire_)
{
}

void TemperatureProbe::begin()
{
  sensors_.begin();
  sensors_.setResolution(Config::Probe::ResolutionBits);
  sensors_.setWaitForConversion(false);
}

void TemperatureProbe::setSettings(const AppSettings &settings)
{
  settings_ = sanitizedSettings(settings);
}

bool TemperatureProbe::update(unsigned long nowMs, float &temperatureC)
{
  if (!conversionPending_) {
    if (!requestSent_ ||
        nowMs - requestedAtMs_ >= settings_.measurementIntervalMs) {
      requestTemperature(nowMs);
    }
    return false;
  }

  if (!conversionReady(nowMs)) {
    return false;
  }

  temperatureC = sensors_.getTempCByIndex(0);
  conversionPending_ = false;
  DEBUG_PRINT("Temperature for device 1: ");
  DEBUG_PRINTLN(temperatureC);
  return true;
}

void TemperatureProbe::recordUpdate()
{
  updateCount_ = nextUpdateCount(updateCount_);
  sensors_.setUserDataByIndex(0, updateCount_);
  (void)sensors_.getUserDataByIndex(0);

  DEBUG_PRINT("Counter: ");
  DEBUG_PRINTLN(updateCount_);
}

int TemperatureProbe::updateCount() const
{
  return updateCount_;
}

void TemperatureProbe::requestTemperature(unsigned long nowMs)
{
  DEBUG_PRINTLN("Requesting temperatures...");
  sensors_.requestTemperatures();
  requestedAtMs_ = nowMs;
  requestSent_ = true;
  conversionPending_ = true;
}

bool TemperatureProbe::conversionReady(unsigned long nowMs) const
{
  return nowMs - requestedAtMs_ >= Config::Probe::ConversionWaitMs;
}
