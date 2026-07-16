#pragma once

#include <Adafruit_SSD1306.h>
#include <IPAddress.h>
#include "config/NetworkConfig.h"
#include "cooling_domain/CoolingController.h"

class OledView {
public:
  OledView();

  void begin();
  void showBootStatus(const char *message,
                      unsigned long elapsedMs = 0,
                      unsigned long totalMs = 0);
  void showStartup();
  void setIpAddress(IPAddress ipAddress);
  void showTemperature(float temperatureC,
                       bool sensorAvailable,
                       int updateCount,
                       const CoolingConfig &coolingConfig,
                       const NetworkConfig &networkConfig,
                       const CoolingOutputs &coolingState);

private:
  static constexpr int DetailPageCount = 4;
  static constexpr int UpdatesPerDetailPage = 6;

  void drawHeader(const CoolingOutputs &coolingState);
  void drawSensorError();
  void drawTemperatureValue(float temperatureC,
                            int updateCount,
                            const CoolingConfig &coolingConfig,
                            const NetworkConfig &networkConfig,
                            const CoolingOutputs &coolingState);
  void drawDetailPage(int page,
                      int updateCount,
                      const CoolingConfig &coolingConfig,
                      const NetworkConfig &networkConfig,
                      const CoolingOutputs &coolingState);
  void drawSettingsPage(const CoolingConfig &config);
  void drawThresholdsPage(const CoolingConfig &config);
  void drawTimingPage(int updateCount, const CoolingConfig &config);
  void drawNetworkPage(const NetworkConfig &config);
  void drawPageMarker(int page);
  void printIpAddress(const IPAddress &ipAddress);
  void printTruncated(const String &text, size_t maxChars);

  Adafruit_SSD1306 display_;
  bool initialized_ = false;
  bool hasAccessPointIp_ = false;
  IPAddress accessPointIp_;
};
