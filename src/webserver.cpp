#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include "httpserver.h"
#include "utils.h"

using std::string;
using std::unique_ptr;
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

  server->Get("/.env", [](const HttpRequest& req, HttpResponse* res) {
    return "FUCK YOU";
  });

}

int main(int argc, char** argv) {
  int port = 80;
  int c;
  bool background = true;
  while ((c = getopt(argc, argv, "p:t")) != -1) {
    switch(c) {
      case 'p':
        if (!Cerver::IsNumber(string(optarg))) {
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
    std::cout << "./httpserver run <directory>\n";
    std::cout << "./httpserver end" << std::endl;
    return EXIT_FAILURE;
  }
  if (string(argv[optind]) == "end") {
    int fd = open("runlog/process.txt", O_RDWR);
    if (fd == -1) {
      std::cout << "No HttpServer is running" << std::endl;
      return EXIT_FAILURE;
    }
    pid_t pid;
    read(fd, &pid, sizeof(pid_t));
    kill(pid, SIGINT);
    close(fd);
    remove("runlog/process.txt");
    return EXIT_SUCCESS;
  }
  if (string(argv[optind]) == "stats") {
     int fd = open("runlog/process.txt", O_RDWR);
    if (fd == -1) {
      std::cout << "No HttpServer is running" << std::endl;
      return EXIT_FAILURE;
    }
    pid_t pid;
    read(fd, &pid, sizeof(pid_t));
    kill(pid, SIGUSR1);
    close(fd);
    return EXIT_SUCCESS;
  }
  if (string(argv[optind]) != "run" || optind + 1 >= argc) {
     std::cout << "./httpserver run <directory>" << std::endl;
    return EXIT_FAILURE;
  }
  dir = string(argv[optind + 1]);
  mkdir("runlog", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  pid_t pid = 0;
  if (background) {
    pid = fork();
  }
  if (pid == 0) {
    server = std::make_unique<HttpServer>(64, port, string(argv[optind + 1]), "runlog");
    DefineGet();
    server->Run();
    if (background) {
      exit(EXIT_SUCCESS);
    }
  }
  int fd = open("runlog/process.txt", O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
  write(fd, &pid, sizeof(pid_t));
  close(fd);
  std::cout << "httpserver is running\n";
  std::cout << "Listenting to port " << port << "\n";
  std::cout << "Reading from directory " << argv[optind + 1] << "\n";
  std::cout << "pid = " << pid << std::endl;
  return EXIT_SUCCESS;
  
}