#include "web/WebHost.h"

WebHost::WebHost(uint16_t port, PageResolver pageResolver)
    : server_(port),
      router_(server_),
      pageResolver_(pageResolver)
{
}

ApiRouter &WebHost::router()
{
  return router_;
}

WebServer &WebHost::server()
{
  return server_;
}

void WebHost::begin()
{
  server_.on("/", [this]() { handlePage(); });
  server_.onNotFound([this]() { handleNotFound(); });
  server_.begin();
}

void WebHost::update()
{
  server_.handleClient();
}

void WebHost::handlePage()
{
  const char *page = pageResolver_ == nullptr ? nullptr
                                               : pageResolver_(server_.uri());
  if (page == nullptr) {
    server_.send(404, "text/plain", "Not found");
    return;
  }
  server_.sendHeader("Cache-Control",
                     "no-store, no-cache, must-revalidate, max-age=0");
  server_.sendHeader("Pragma", "no-cache");
  server_.send_P(200, "text/html; charset=utf-8", page);
}

void WebHost::handleNotFound()
{
  const char *page = pageResolver_ == nullptr ? nullptr
                                               : pageResolver_(server_.uri());
  if (page != nullptr) {
    server_.sendHeader("Cache-Control",
                       "no-store, no-cache, must-revalidate, max-age=0");
    server_.sendHeader("Pragma", "no-cache");
    server_.send_P(200, "text/html; charset=utf-8", page);
    return;
  }
  server_.send(404, "text/plain", "Not found");
}
