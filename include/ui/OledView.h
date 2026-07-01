#pragma once

#include <Adafruit_SSD1306.h>
#include <IPAddress.h>
#include "domain/AppSettings.h"
#include "domain/CoolingController.h"

class OledView {
public:
  OledView();

  void begin();
  void showStartup();
  void setIpAddress(IPAddress ipAddress);
  void showTemperature(float temperatureC,
                       int updateCount,
                       const AppSettings &settings,
                       const CoolingState &coolingState);

private:
  static constexpr int DetailPageCount = 4;
  static constexpr int UpdatesPerDetailPage = 6;

  void drawHeader(const CoolingState &coolingState);
  void drawSensorError();
  void drawTemperatureValue(float temperatureC,
                            int updateCount,
                            const AppSettings &settings,
                            const CoolingState &coolingState);
  void drawDetailPage(int page,
                      int updateCount,
                      const AppSettings &settings,
                      const CoolingState &coolingState);
  void drawSettingsPage(const AppSettings &settings);
  void drawThresholdsPage(const AppSettings &settings);
  void drawTimingPage(int updateCount, const AppSettings &settings);
  void drawNetworkPage(const AppSettings &settings);
  void drawPageMarker(int page);
  void printIpAddress(const IPAddress &ipAddress);
  void printTruncated(const String &text, size_t maxChars);

  Adafruit_SSD1306 display_;
  bool initialized_ = false;
  bool hasAccessPointIp_ = false;
  IPAddress accessPointIp_;
};
