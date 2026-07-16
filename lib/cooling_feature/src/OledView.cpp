#include "ui/OledView.h"

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "config/Config.h"
#include "debug/DebugLog.h"

OledView::OledView()
    : display_(Config::Screen::Width,
               Config::Screen::Height,
               &Wire,
               Config::Screen::ResetPin)
{
}

void OledView::begin()
{
  Wire.begin(Config::Pins::I2cSda, Config::Pins::I2cScl);

  initialized_ = display_.begin(SSD1306_SWITCHCAPVCC,
                                Config::Screen::Address);
  if (!initialized_) {
    DEBUG_PRINTLN(F("SSD1306 allocation failed"));
  }
}

void OledView::showBootStatus(const char *message,
                              unsigned long elapsedMs,
                              unsigned long totalMs)
{
  if (!initialized_) {
    return;
  }

  const int progressWidth =
      totalMs > 0
          ? static_cast<int>((elapsedMs * 86UL) / totalMs)
          : 0;

  display_.clearDisplay();
  display_.drawRoundRect(2, 2, 24, 24, 4, SSD1306_WHITE);
  display_.fillCircle(9, 11, 3, SSD1306_WHITE);
  display_.drawLine(16, 8, 21, 8, SSD1306_WHITE);
  display_.drawLine(16, 13, 21, 13, SSD1306_WHITE);
  display_.drawLine(7, 21, 21, 21, SSD1306_WHITE);

  display_.setTextSize(1);
  display_.setTextColor(SSD1306_WHITE);
  display_.setCursor(34, 4);
  display_.println("Cooling");
  display_.setCursor(34, 14);
  display_.println("Controller");

  display_.drawLine(0, 31, Config::Screen::Width, 31, SSD1306_WHITE);

  display_.setCursor(0, 37);
  display_.print(message);

  display_.drawRect(0, 54, 88, 7, SSD1306_WHITE);
  if (progressWidth > 0) {
    display_.fillRect(1, 55, min(progressWidth, 86), 5, SSD1306_WHITE);
  }

  if (totalMs > 0) {
    display_.setCursor(94, 53);
    display_.print((elapsedMs + 999UL) / 1000UL);
    display_.print("/");
    display_.print((totalMs + 999UL) / 1000UL);
    display_.print("s");
  }

  display_.display();
}

void OledView::showStartup()
{
  if (!initialized_) {
    return;
  }

  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setTextColor(SSD1306_WHITE);
  display_.setCursor(0, 0);
  display_.println("Init...");
  display_.setCursor(0, 12);
  display_.println("Relais test");
  if (hasAccessPointIp_) {
    display_.setCursor(0, 24);
    display_.print("AP ");
    printIpAddress(accessPointIp_);
  }
  display_.display();
}

void OledView::setIpAddress(IPAddress ipAddress)
{
  accessPointIp_ = ipAddress;
  hasAccessPointIp_ = true;
}

void OledView::showTemperature(float temperatureC,
                               bool sensorAvailable,
                               int updateCount,
                               const CoolingConfig &coolingConfig,
                               const NetworkConfig &networkConfig,
                               const CoolingOutputs &coolingState)
{
  if (!initialized_) {
    return;
  }

  drawHeader(coolingState);

  if (!sensorAvailable) {
    drawSensorError();
  } else {
    drawTemperatureValue(temperatureC,
                         updateCount,
                         coolingConfig,
                         networkConfig,
                         coolingState);
  }

  display_.display();
}

void OledView::drawHeader(const CoolingOutputs &coolingState)
{
  display_.clearDisplay();
  display_.fillRect(0, 0, Config::Screen::Width, 10, SSD1306_WHITE);

  display_.setTextSize(1);
  display_.setTextColor(SSD1306_BLACK);
  display_.setCursor(2, 1);
  if (coolingState.fanRunOnActive) {
    display_.print("NACHLAUF ");
    display_.print((coolingState.fanRunOnRemainingMs + 999) / 1000);
    display_.print("s");
  } else {
    display_.print(coolingState.fanEnabled ? "KUEHLUNG AKTIV"
                                           : "KUEHLUNG AUS");
  }

  display_.setTextColor(SSD1306_WHITE);
}

void OledView::drawSensorError()
{
  display_.setTextSize(1);
  display_.setCursor(0, 14);
  display_.println("Sensorfehler");

  display_.setTextSize(2);
  display_.setCursor(0, 26);
  display_.println("-127 C");

  display_.setTextSize(1);
  display_.setCursor(0, 48);
  display_.println("Relais AUS | Pull-Up?");
}

void OledView::drawTemperatureValue(float temperatureC,
                                    int updateCount,
                                    const CoolingConfig &coolingConfig,
                                    const NetworkConfig &networkConfig,
                                    const CoolingOutputs &coolingState)
{
  const int page =
      (updateCount / UpdatesPerDetailPage) % DetailPageCount;
  if (page == 3) {
    drawNetworkPage(networkConfig);
    drawPageMarker(page);
    return;
  }

  display_.setTextSize(2);
  display_.setCursor(0, 14);
  display_.print(temperatureC, 1);

  display_.setTextSize(1);
  display_.print(" C");

  display_.setCursor(70, 14);
  display_.print("P:");
  display_.println(coolingState.peltierEnabled ? "EIN" : "AUS");
  display_.setCursor(70, 24);
  display_.print("F:");
  display_.println(coolingState.fanEnabled ? "EIN" : "AUS");

  drawDetailPage(page,
                 updateCount,
                 coolingConfig,
                 networkConfig,
                 coolingState);
}

void OledView::drawDetailPage(int page,
                              int updateCount,
                              const CoolingConfig &coolingConfig,
                              const NetworkConfig &networkConfig,
                              const CoolingOutputs &coolingState)
{
  (void)coolingState;
  switch (page) {
  case 1:
    drawThresholdsPage(coolingConfig);
    break;
  case 2:
    drawTimingPage(updateCount, coolingConfig);
    break;
  case 3:
    drawNetworkPage(networkConfig);
    break;
  default:
    drawSettingsPage(coolingConfig);
    break;
  }

  drawPageMarker(page);
}

void OledView::drawSettingsPage(const CoolingConfig &settings)
{
  display_.setCursor(0, 35);
  display_.print("Soll ");
  display_.print(settings.targetTemperatureC, 1);
  display_.print("C");

  display_.setCursor(0, 45);
  display_.print("Hys ");
  display_.print(settings.hysteresisC, 1);
  display_.print("C");

  display_.setCursor(0, 55);
  display_.print("Mess ");
  display_.print(settings.measurementIntervalMs);
  display_.print("ms");
}

void OledView::drawThresholdsPage(const CoolingConfig &settings)
{
  display_.setCursor(0, 35);
  display_.print("Schwellen");

  display_.setCursor(0, 45);
  display_.print("Ein  >");
  display_.print(settings.targetTemperatureC +
                 settings.hysteresisC,
                 1);
  display_.print("C");
  display_.setCursor(0, 55);
  display_.print("Aus <=");
  display_.print(settings.targetTemperatureC, 1);
  display_.print("C");
}

void OledView::drawTimingPage(int updateCount, const CoolingConfig &settings)
{
  display_.setCursor(0, 35);
  display_.print("Mess ");
  display_.print(settings.measurementIntervalMs);
  display_.print("ms");

  display_.setCursor(0, 45);
  display_.print("Fan+ ");
  display_.print(settings.fanRunOnMs / 1000);
  display_.print("s");

  display_.setCursor(0, 55);
  display_.print("Upd  ");
  display_.print(updateCount);
}

void OledView::drawNetworkPage(const NetworkConfig &settings)
{
  display_.setTextSize(1);
  display_.setCursor(0, 14);
  display_.print("AP  ");
  printTruncated(Config::Network::AccessPointSsid, 16);

  display_.setCursor(0, 24);
  display_.print("IP  ");
  if (hasAccessPointIp_) {
    printIpAddress(accessPointIp_);
  } else {
    display_.print("--");
  }

  display_.setCursor(0, 38);
  display_.print("STA ");
  if (settings.stationSsid.length() > 0) {
    printTruncated(settings.stationSsid, 16);
  } else {
    display_.print("OFF");
  }

  display_.setCursor(0, 48);
  display_.print("IP  ");
  if (settings.stationSsid.length() == 0) {
    display_.print("--");
  } else if (WiFi.status() == WL_CONNECTED) {
    printIpAddress(WiFi.localIP());
  } else {
    display_.print("NO LINK");
  }
}

void OledView::drawPageMarker(int page)
{
  constexpr int DotX = 122;
  constexpr int DotY = 42;
  constexpr int DotStep = 6;

  for (int i = 0; i < DetailPageCount; ++i) {
    if (i == page) {
      display_.fillRect(DotX, DotY + i * DotStep, 4, 4, SSD1306_WHITE);
    } else {
      display_.drawRect(DotX, DotY + i * DotStep, 4, 4, SSD1306_WHITE);
    }
  }
}

void OledView::printIpAddress(const IPAddress &ipAddress)
{
  display_.print(ipAddress[0]);
  display_.print(".");
  display_.print(ipAddress[1]);
  display_.print(".");
  display_.print(ipAddress[2]);
  display_.print(".");
  display_.print(ipAddress[3]);
}

void OledView::printTruncated(const String &text, size_t maxChars)
{
  if (text.length() <= maxChars) {
    display_.print(text);
    return;
  }

  display_.print(text.substring(0, maxChars - 1));
  display_.print("~");
}
