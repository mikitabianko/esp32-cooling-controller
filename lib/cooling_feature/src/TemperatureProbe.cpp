#include "hardware/TemperatureProbe.h"

#include <Arduino.h>
#include "config/Config.h"
#include "debug/DebugLog.h"

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

void TemperatureProbe::setConfig(const TemperatureProbeConfig &config)
{
  config_ = config;
}

bool TemperatureProbe::update(uint32_t nowMs, TemperatureReading &reading)
{
  if (!conversionPending_) {
    if (!requestSent_ ||
        nowMs - requestedAtMs_ >= config_.measurementIntervalMs) {
      requestTemperature(nowMs);
    }
    return false;
  }

  if (!conversionReady(nowMs)) {
    return false;
  }

  reading.temperatureC = sensors_.getTempCByIndex(0);
  reading.sensorAvailable = reading.temperatureC != DEVICE_DISCONNECTED_C;
  conversionPending_ = false;
  DEBUG_PRINT("Temperature for device 1: ");
  DEBUG_PRINTLN(reading.temperatureC);
  return true;
}

void TemperatureProbe::recordUpdate()
{
  ++updateCount_;
  sensors_.setUserDataByIndex(0, updateCount_);
  (void)sensors_.getUserDataByIndex(0);

  DEBUG_PRINT("Counter: ");
  DEBUG_PRINTLN(updateCount_);
}

int TemperatureProbe::updateCount() const
{
  return updateCount_;
}

void TemperatureProbe::requestTemperature(uint32_t nowMs)
{
  DEBUG_PRINTLN("Requesting temperatures...");
  sensors_.requestTemperatures();
  requestedAtMs_ = nowMs;
  requestSent_ = true;
  conversionPending_ = true;
}

bool TemperatureProbe::conversionReady(uint32_t nowMs) const
{
  return nowMs - requestedAtMs_ >= Config::Probe::ConversionWaitMs;
}
