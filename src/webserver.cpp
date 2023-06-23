#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include "httpserver.h"
#include "utils.h"
#include "routes.h"

using std::string;
using std::unique_ptr;
using namespace Cerver;

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
  if (background) {
    pid_t pid = 0;
    pid = fork();
    if (pid == 0) {
      server = std::make_unique<HttpServer>(32, 80);
      DefineGet();
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
    DefineGet();
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