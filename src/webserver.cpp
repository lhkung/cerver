#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include "httpserver.h"
#include "utils.h"
#include "tabula/tabula.h"

using std::string;
using std::unique_ptr;
using namespace Cerver;
using namespace KVStore;

static string dir;

void LoadFileToDatabase(const string& dir, KVStore::Tabula* tabula) {
  HttpResponse res;
  HttpServer::ReadFile(&res, dir + "/index.html");
  tabula->Put("assets", "page", "index.html", res.Body());
  HttpServer::ReadFile(&res, dir + "/poetry.html");
  tabula->Put("assets", "page", "poetry.html", res.Body());
  HttpServer::ReadFile(&res, dir + "/translated.html");
  tabula->Put("assets", "page", "translated.html", res.Body());
  HttpServer::ReadFile(&res, dir + "/map.html");
  tabula->Put("assets", "page", "map.html", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/allegory.png");
  tabula->Put("assets", "image", "allegory.png", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/bookshelf.png");
  tabula->Put("assets", "image", "bookshelf.png", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/CA-Chinese.jpeg");
  tabula->Put("assets", "image", "CA-Chinese.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/CA-English.jpeg");
  tabula->Put("assets", "image", "CA-English.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/favicon.png");
  tabula->Put("assets", "image", "favicon.png", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/HNTDA-Chinese.jpeg");
  tabula->Put("assets", "image", "HNTDA-Chinese.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/HNTDA-English.jpeg");
  tabula->Put("assets", "image", "HNTDA-English.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/HOAX-Chinese.jpeg");
  tabula->Put("assets", "image", "HOAX-Chinese.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/HOAX-English.jpeg");
  tabula->Put("assets", "image", "HOAX-English.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/HST-Chinese.jpeg");
  tabula->Put("assets", "image", "HST-Chinese.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/HST-English.jpeg");
  tabula->Put("assets", "image", "HST-English.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/kung.png");
  tabula->Put("assets", "image", "kung.png", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/network.png");
  tabula->Put("assets", "image", "network.png", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/programing.png");
  tabula->Put("assets", "image", "programing.png", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/right-arrow.png");
  tabula->Put("assets", "image", "right-arrow.png", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/TAW-Chinese.jpeg");
  tabula->Put("assets", "image", "TAW-Chinese.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/TAW-English.jpeg");
  tabula->Put("assets", "image", "TAW-English.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/TGA-Chinese.jpeg");
  tabula->Put("assets", "image", "TGA-Chinese.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/TGA-English.jpeg");
  tabula->Put("assets", "image", "TGA-English.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/translating.png");
  tabula->Put("assets", "image", "translating.png", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/TTM-Chinese.jpeg");
  tabula->Put("assets", "image", "TTM-Chinese.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/images/TTM-English.jpeg");
  tabula->Put("assets", "image", "TTM-English.jpeg", res.Body());
  HttpServer::ReadFile(&res, dir + "/css/style.css");
  tabula->Put("assets", "css", "style.css", res.Body());
}

void DefineGet(KVStore::Tabula* tabula) {
  server->Get("/", [tabula](const HttpRequest& req, HttpResponse* res) {
    tabula->Get("assets", "page", "index.html", res->BodyPtr());
    res->UseBody();
    return "";
  });

  server->Get("/poetry", [tabula](const HttpRequest& req, HttpResponse* res) {
    tabula->Get("assets", "page", "poetry.html", res->BodyPtr());
    res->UseBody();
    return "";
  });

  server->Get("/translated", [tabula](const HttpRequest& req, HttpResponse* res) {
    tabula->Get("assets", "page", "translated.html", res->BodyPtr());
    res->UseBody();
    return "";
  });

  server->Get("/travel", [tabula](const HttpRequest& req, HttpResponse* res) {
    tabula->Get("assets", "page", "map.html", res->BodyPtr());
    res->UseBody();
    return "";
  });

  server->Get("/images/:imageFile", [tabula](const HttpRequest& req, HttpResponse* res) {
    string image_file;
    req.PathParam("imageFile", &image_file);
    tabula->Get("assets", "image", image_file, res->BodyPtr());
    res->UseBody();
    return "";
  });

  server->Get("/CSS/:cssFile", [tabula](const HttpRequest& req, HttpResponse* res) {
    string css_file;
    req.PathParam("cssFile", &css_file);
    tabula->Get("assets", "css", css_file, res->BodyPtr());
    res->UseBody();
    return "";
  });

  server->Get("/stats", [](const HttpRequest& req, HttpResponse* res) {
    server->GetStats(req, res);
    return "";
  });
}

int main(int argc, char** argv) {
  int port = 80;
  int c;
  bool background = true;
  while ((c = getopt(argc, argv, "p:t")) != -1) {
    switch(c) {
      case 'p':
        if (!Utils::IsNumber(string(optarg))) {
          std::cout << "-p argument must be a number that specifies a port" << std::endl;
          return EXIT_FAILURE;
        }
        port = atoi(optarg);
        break;
      case 't':
        background = false;
        break;
      case '?':
        std::cout << optopt << " is not an accepted argument." << std::endl;
        return 1;
      default:
        abort();
    }
  }
  if (optind >= argc) {
    std::cout << "./webserver run\n./webserver end" << std::endl;
    return EXIT_FAILURE;
  }
  if (string(argv[optind]) == "end") {
    int fd = open("cerverlog/process.txt", O_RDWR);
    if (fd == -1) {
      std::cout << "No cerver is running" << std::endl;
      return EXIT_FAILURE;
    }
    pid_t pid;
    read(fd, &pid, sizeof(pid_t));
    kill(pid, SIGINT);
    close(fd);
    remove("cerverlog/process.txt");
    return EXIT_SUCCESS;
  }
  if (string(argv[optind]) == "stats") {
     int fd = open("cerverlog/process.txt", O_RDWR);
    if (fd == -1) {
      std::cout << "No cerver is running" << std::endl;
      return EXIT_FAILURE;
    }
    pid_t pid;
    read(fd, &pid, sizeof(pid_t));
    kill(pid, SIGUSR1);
    close(fd);
    return EXIT_SUCCESS;
  }
  if (string(argv[optind]) != "run" || optind + 1 >= argc) {
     std::cout << "./webserver run <directory>" << std::endl;
    return EXIT_FAILURE;
  }
  dir = string(argv[optind + 1]);
  unique_ptr<KVStore::Tabula> tabula = std::make_unique<KVStore::Tabula>(dir);
  if (background) {
    pid_t pid = 0;
    pid = fork();
    if (pid == 0) {
      server = std::make_unique<HttpServer>(32, 80);
      LoadFileToDatabase(dir, tabula.get());
      // tabula->Recover("/Users/seankung/projects/cerver/assets/tabula-data");
      DefineGet(tabula.get());
      server->Run();
      exit(EXIT_SUCCESS);
    } else {
      int fd = open("cerverlog/process.txt", O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
      write(fd, &pid, sizeof(pid_t));
      close(fd);
      std::cout << "Webserver is running\n";
      std::cout << "Listenting on port " << port << "\n";
      std::cout << "Reading from directory " << argv[optind + 1] << "\n";
      std::cout << "pid = " << pid << std::endl;
      return EXIT_SUCCESS;
    }
  } else {
    server = std::make_unique<HttpServer>(32, 80);
    LoadFileToDatabase(dir, tabula.get());
    // tabula->Recover("/Users/seankung/projects/cerver/assets/data");
    DefineGet(tabula.get());
    pid_t pid = getpid();
    int fd = open("cerverlog/process.txt", O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
    write(fd, &pid, sizeof(pid_t));
    close(fd);
    std::cout << "Webserver is running\n";
    std::cout << "Listenting on port " << port << "\n";
    std::cout << "Reading from directory " << argv[optind + 1] << "\n";
    std::cout << "pid = " << pid << std::endl;
    server->Run();
    return EXIT_SUCCESS;
  }
  
}