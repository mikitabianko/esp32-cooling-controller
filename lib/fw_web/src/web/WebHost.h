#pragma once

#include <WebServer.h>

#include "web/ApiRouter.h"

class WebHost {
public:
  using PageResolver = const char *(*)(const String &path);

  WebHost(uint16_t port, PageResolver pageResolver);

  ApiRouter &router();
  WebServer &server();
  void begin();
  void update();

private:
  void handlePage();
  void handleNotFound();

  WebServer server_;
  ApiRouter router_;
  PageResolver pageResolver_;
};
