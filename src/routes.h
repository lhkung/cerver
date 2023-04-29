#include "httpserver.h"


using std::string;
using namespace Cerver;

static string dir;

void DefineGet() {
  server->Get("/", [](const HttpRequest& req, HttpResponse* res) {
    HttpServer::ReadFile(req, res, dir + "/index.html");
    return "";
  });

  server->Get("/poetry", [](const HttpRequest& req, HttpResponse* res) {
    HttpServer::ReadFile(req, res, dir + "/poetry.html");
    return "";
  });

  server->Get("/translated", [](const HttpRequest& req, HttpResponse* res) {
    HttpServer::ReadFile(req, res, dir + "/translated.html");
    return "";
  });

  server->Get("/travel", [](const HttpRequest& req, HttpResponse* res) {
    HttpServer::ReadFile(req, res, dir + "/map.html");
    return "";
  });

  server->Get("/images/:imageFile", [](const HttpRequest& req, HttpResponse* res) {
    string image_file;
    req.PathParam("imageFile", &image_file);
    HttpServer::ReadFile(req, res, dir + "/images/" + image_file);
    return "";
  });

  server->Get("/CSS/:cssFile", [](const HttpRequest& req, HttpResponse* res) {
    string css_file;
    req.PathParam("cssFile", &css_file);
    HttpServer::ReadFile(req, res, dir + "/CSS/" + css_file);
    return "";
  });

}