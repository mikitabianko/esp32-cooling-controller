#pragma once

#include <Arduino.h>

#ifndef COOLING_OTA_ENABLED
#define COOLING_OTA_ENABLED 1
#endif

#ifndef COOLING_OTA_HOSTNAME
#define COOLING_OTA_HOSTNAME "cooling-controller"
#endif

#ifndef COOLING_OTA_PASSWORD
#define COOLING_OTA_PASSWORD ""
#endif

#ifndef COOLING_OTA_PASSWORD_HASH
#define COOLING_OTA_PASSWORD_HASH ""
#endif

namespace Config {
namespace Pins {
constexpr uint8_t I2cSda = 21;
constexpr uint8_t I2cScl = 22;
constexpr uint8_t OneWireBus = 26;
constexpr uint8_t PeltierRelay = 17; // IN1
constexpr uint8_t FanRelay = 16;     // IN2
} // namespace Pins

namespace Relay {
constexpr uint8_t OnLevel = LOW;
constexpr uint8_t OffLevel = HIGH;
constexpr unsigned long SelfTestPulseMs = 500;
} // namespace Relay

namespace Probe {
constexpr uint8_t ResolutionBits = 12;
constexpr unsigned long ConversionWaitMs = 750;
} // namespace Probe

namespace Network {
constexpr const char *AccessPointSsid = "CoolingController";
constexpr const char *AccessPointPassword = "cooling123";
constexpr const char *StationSsid = "";
constexpr const char *StationPassword = "";
constexpr unsigned long StationConnectTimeoutMs = 15000;
constexpr unsigned long StationReconnectIntervalMs = 30000;
constexpr size_t MaxStationSsidLength = 32;
constexpr size_t MaxStationPasswordLength = 64;
constexpr uint16_t WebServerPort = 80;
} // namespace Network

namespace Ota {
constexpr bool Enabled = COOLING_OTA_ENABLED;
constexpr const char *Hostname = COOLING_OTA_HOSTNAME;
constexpr const char *Password = COOLING_OTA_PASSWORD;
constexpr const char *PasswordHash = COOLING_OTA_PASSWORD_HASH;
} // namespace Ota

namespace Debug {
constexpr unsigned long SerialBaud = 115200;
} // namespace Debug

namespace Control {
constexpr unsigned long DefaultMeasurementIntervalMs = 500;
constexpr float DefaultTargetTemperatureC = 5.0F;
constexpr float DefaultHysteresisC = 0.1F;
constexpr unsigned long DefaultFanRunOnMs = 30000;

constexpr unsigned long MinMeasurementIntervalMs = 250;
constexpr unsigned long MaxMeasurementIntervalMs = 60000;
constexpr float MinTargetTemperatureC = -20.0F;
constexpr float MaxTargetTemperatureC = 40.0F;
constexpr float MinHysteresisC = 0.1F;
constexpr float MaxHysteresisC = 10.0F;
constexpr unsigned long MinFanRunOnMs = 0;
constexpr unsigned long MaxFanRunOnMs = 600000;
} // namespace Control

namespace Screen {
constexpr int Width = 128;
constexpr int Height = 64;
constexpr int ResetPin = -1;
constexpr uint8_t Address = 0x3C;

constexpr int TitleX = 10;
constexpr int TitleY = 8;
constexpr int DividerY = 20;
constexpr int ContentX = 15;
constexpr int TemperatureY = 28;
constexpr int HintY = 42;
constexpr int FooterY = 54;
} // namespace Screen
} // namespace Config
