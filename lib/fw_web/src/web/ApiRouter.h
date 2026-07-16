#pragma once

#include <WebServer.h>

class ApiRouter {
public:
  explicit ApiRouter(WebServer &server)
      : server_(server)
  {
  }

  WebServer &server() { return server_; }

  template <typename Handler>
  void any(const char *path, Handler handler)
  {
    server_.on(path, handler);
  }

  template <typename Handler>
  void get(const char *path, Handler handler)
  {
    server_.on(path, HTTP_GET, handler);
  }

  template <typename Handler>
  void post(const char *path, Handler handler)
  {
    server_.on(path, HTTP_POST, handler);
  }

  template <typename Handler, typename UploadHandler>
  void upload(const char *path, Handler handler, UploadHandler uploadHandler)
  {
    server_.on(path, HTTP_POST, handler, uploadHandler);
  }

private:
  WebServer &server_;
};
