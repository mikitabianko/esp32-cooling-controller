#include "ota/FirmwareUpdateService.h"

#include <Update.h>

#include "core/TimeMath.h"
#include "debug/DebugLog.h"
#include "web/JsonHelpers.h"

FirmwareUpdateService::FirmwareUpdateService(WebServer &server,
                                             SystemControl &system)
    : server_(server),
      system_(system)
{
}

void FirmwareUpdateService::registerApi(ApiRouter &router)
{
  router.upload(
      "/api/firmware",
      [this]() { handleUploadComplete(); },
      [this]() { handleUploadStream(); });
}

void FirmwareUpdateService::update(uint32_t nowMs)
{
  if (restartPending_ && deadlineReached(nowMs, restartAtMs_)) {
    system_.restart();
  }
}

void FirmwareUpdateService::handleUploadComplete()
{
  if (!uploadOk_) {
    String json = "{\"ok\":false,\"error\":";
    json += JsonHelpers::escapedString(
        uploadError_.length() > 0 ? uploadError_
                                  : String("firmware upload failed"));
    json += "}";
    server_.send(500, "application/json", json);
    return;
  }

  restartPending_ = true;
  restartAtMs_ = millis() + 1200U;
  server_.send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
}

void FirmwareUpdateService::handleUploadStream()
{
  HTTPUpload &upload = server_.upload();
  if (upload.status == UPLOAD_FILE_START) {
    uploadOk_ = false;
    uploadError_ = "";
    DEBUG_PRINT("HTTP firmware update started: ");
    DEBUG_PRINTLN(upload.filename);
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      uploadError_ = Update.errorString();
      DEBUG_PRINT("HTTP firmware update failed: ");
      DEBUG_PRINTLN(uploadError_);
      return;
    }
    uploadOk_ = true;
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (!uploadOk_) {
      return;
    }
    const size_t written = Update.write(upload.buf, upload.currentSize);
    if (written != upload.currentSize) {
      uploadOk_ = false;
      uploadError_ = Update.errorString();
      Update.abort();
      DEBUG_PRINT("HTTP firmware update failed: ");
      DEBUG_PRINTLN(uploadError_);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (!uploadOk_) {
      return;
    }
    if (!Update.end(true)) {
      uploadOk_ = false;
      uploadError_ = Update.errorString();
      DEBUG_PRINT("HTTP firmware update failed: ");
      DEBUG_PRINTLN(uploadError_);
      return;
    }
    DEBUG_PRINT("HTTP firmware update completed: ");
    DEBUG_PRINT(upload.totalSize);
    DEBUG_PRINTLN(" bytes");
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    uploadOk_ = false;
    uploadError_ = "upload aborted";
    Update.abort();
    DEBUG_PRINTLN("HTTP firmware update failed: upload aborted");
  }
}
