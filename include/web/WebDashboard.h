#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#include <WebServer.h>
#include "domain/AppSettings.h"
#include "domain/CoolingController.h"

struct DashboardSnapshot {
  float temperatureC = 0.0F;
  bool hasTemperature = false;
  bool sensorDisconnected = false;
  int updateCount = 0;
  CoolingState coolingState;
  AppSettings settings;
  unsigned long uptimeMs = 0;
};

struct DevDashboardState {
  bool enabled = false;
  float temperatureC = 5.0F;
  bool hasTemperature = true;
  bool sensorDisconnected = false;
  int updateCount = 0;
  CoolingState coolingState;
};

class WebDashboard {
public:
  WebDashboard();

  IPAddress begin();
  void update();
  void setSnapshot(const DashboardSnapshot &snapshot);
  void setSettings(const AppSettings &settings);
  bool takePendingSettings(AppSettings &settings);
  IPAddress ipAddress() const;

private:
  void handlePage();
  void handleStatus();
  void handleGetDevState();
  void handleSaveDevState();
  void handleGetSettings();
  void handleSaveSettings();
  void handleNotFound();
  String statusJson() const;
  String settingsJson() const;
  String devJson() const;
  String jsonString(const String &value) const;
  String boolText(bool value) const;
  String stationStatusText() const;
  void connectStation();
  void maintainStationConnection();
  bool readSettingsArgs(AppSettings &settings);
  bool readDevArgs(DevDashboardState &state);
  bool readBoolArg(const char *name, bool &value);
  bool readStringArg(const char *name, String &value);
  bool readIntArg(const char *name, int &value);
  bool readFloatArg(const char *name, float &value);
  bool readUnsignedLongArg(const char *name, unsigned long &value);
  DashboardSnapshot effectiveSnapshot() const;

  WebServer server_;
  IPAddress ipAddress_;
  DashboardSnapshot snapshot_;
  DevDashboardState devState_;
  AppSettings settings_;
  AppSettings pendingSettings_;
  bool hasPendingSettings_ = false;
  bool networkStarted_ = false;
  unsigned long lastStationConnectAttemptMs_ = 0;
};
