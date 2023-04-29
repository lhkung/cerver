#include "httpserver.h"

using namespace Cerver;

int main(int argc, char** argv) {
  server = std::make_unique<HttpServer>(32, 80);
  server->Get("/", [](const HttpRequest& req, HttpResponse* res) {
    return "Hello World";
  });
  server->Run();
  return 0;
}