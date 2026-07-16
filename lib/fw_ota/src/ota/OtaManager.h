#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>

struct OtaManagerConfig {
  bool enabled = true;
  const char *hostname = "device";
  const char *password = "";
  const char *passwordHash = "";
  unsigned long serialBaud = 115200U;
};

struct OtaStatus {
  bool enabled = false;
  bool started = false;
  bool updating = false;
  unsigned int progressPercent = 0;
  const char *state = "disabled";
  const char *lastError = "";
};

class OtaManager {
public:
  explicit OtaManager(const OtaManagerConfig &config);

  void beginIfReady();
  void update();
  bool started() const;
  OtaStatus status() const;

private:
  void configureCallbacks();
  void markUpdateStarted();
  void markProgress(unsigned int progress, unsigned int total);
  void markUpdateCompleted();
  void markUpdateFailed(ota_error_t error);

  static OtaManager *active_;
  OtaManagerConfig config_;
  bool started_ = false;
  bool updating_ = false;
  unsigned int progressPercent_ = 0;
  const char *state_ = "waiting";
  const char *lastError_ = "";
};
