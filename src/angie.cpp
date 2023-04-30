#include "httpserver.h"

int main(int argc, char** argv) {
  Cerver::server->Get("/", [](const Cerver::HttpRequest& req, Cerver::HttpResponse* res) {
    return "Hello World";
  });
  Cerver::server->Run();
  return 0;
}