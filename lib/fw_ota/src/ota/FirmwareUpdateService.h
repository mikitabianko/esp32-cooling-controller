#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "core/SystemControl.h"
#include "web/ApiRouter.h"

class FirmwareUpdateService {
public:
  FirmwareUpdateService(WebServer &server, SystemControl &system);

  void registerApi(ApiRouter &router);
  void update(uint32_t nowMs);

private:
  void handleUploadComplete();
  void handleUploadStream();

  WebServer &server_;
  SystemControl &system_;
  bool uploadOk_ = false;
  bool restartPending_ = false;
  uint32_t restartAtMs_ = 0U;
  String uploadError_;
};
