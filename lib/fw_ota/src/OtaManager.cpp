#include "ota/OtaManager.h"

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <cstring>

namespace {

const char *errorReason(ota_error_t error)
{
  switch (error) {
  case OTA_AUTH_ERROR:
    return "authentication failed";
  case OTA_BEGIN_ERROR:
    return "begin failed";
  case OTA_CONNECT_ERROR:
    return "connection failed";
  case OTA_RECEIVE_ERROR:
    return "receive failed";
  case OTA_END_ERROR:
    return "finalization failed";
  default:
    return "unknown error";
  }
}

void ensureSerialLogging(unsigned long serialBaud)
{
  Serial.begin(serialBaud);
}

} // namespace

OtaManager *OtaManager::active_ = nullptr;

OtaManager::OtaManager(const OtaManagerConfig &config)
    : config_(config)
{
}

void OtaManager::beginIfReady()
{
  if (!config_.enabled || started_ || WiFi.status() != WL_CONNECTED) {
    return;
  }

  ensureSerialLogging(config_.serialBaud);
  active_ = this;
  configureCallbacks();

  ArduinoOTA.setHostname(config_.hostname);
  if (strlen(config_.passwordHash) > 0) {
    ArduinoOTA.setPasswordHash(config_.passwordHash);
  } else if (strlen(config_.password) > 0) {
    ArduinoOTA.setPassword(config_.password);
  }

  ArduinoOTA.begin();
  started_ = true;
  state_ = "ready";
  lastError_ = "";

  Serial.print("OTA service started: ");
  Serial.print(config_.hostname);
  Serial.print(" at ");
  Serial.println(WiFi.localIP());
}

void OtaManager::update()
{
  if (!config_.enabled || !started_) {
    return;
  }

  ArduinoOTA.handle();
}

bool OtaManager::started() const
{
  return started_;
}

OtaStatus OtaManager::status() const
{
  OtaStatus status;
  status.enabled = config_.enabled;
  status.started = started_;
  status.updating = updating_;
  status.progressPercent = progressPercent_;
  status.state = config_.enabled ? state_ : "disabled";
  status.lastError = lastError_;
  return status;
}

void OtaManager::configureCallbacks()
{
  ArduinoOTA.onStart([]() {
    if (active_ != nullptr) {
      active_->markUpdateStarted();
    }
    Serial.print("OTA update started: ");
    Serial.println(ArduinoOTA.getCommand() == U_FLASH ? "firmware"
                                                       : "filesystem");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned int lastPercent = 101;
    const unsigned int percent = total == 0 ? 0 : (progress * 100U) / total;
    if (active_ != nullptr) {
      active_->markProgress(progress, total);
    }
    if (percent != lastPercent && (percent == 100 || percent % 5 == 0)) {
      lastPercent = percent;
      Serial.print("OTA update progress: ");
      Serial.print(percent);
      Serial.println("%");
    }
  });

  ArduinoOTA.onEnd([]() {
    if (active_ != nullptr) {
      active_->markUpdateCompleted();
    }
    Serial.println("OTA update completed");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    if (active_ != nullptr) {
      active_->markUpdateFailed(error);
    }
    Serial.print("OTA update failed: ");
    Serial.println(errorReason(error));
  });
}

void OtaManager::markUpdateStarted()
{
  updating_ = true;
  progressPercent_ = 0;
  state_ = "updating";
  lastError_ = "";
}

void OtaManager::markProgress(unsigned int progress, unsigned int total)
{
  progressPercent_ = total == 0 ? 0 : (progress * 100U) / total;
}

void OtaManager::markUpdateCompleted()
{
  updating_ = false;
  progressPercent_ = 100;
  state_ = "completed";
  lastError_ = "";
}

void OtaManager::markUpdateFailed(ota_error_t error)
{
  updating_ = false;
  state_ = "failed";
  lastError_ = errorReason(error);
}
