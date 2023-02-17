#ifndef LOGGER_H_
#define LOGGER_H_
#include <string>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <ctime>

namespace Cerver {

class Logger {
    public:
      Logger(std::string dir, size_t max_size) : max_size_(max_size) {
        std::string path = dir + "/run.log";
        logfd_ = open(path.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
        if (logfd_ == -1) {
          std::cout << errno << std::endl;
        }
      }
      virtual ~Logger() { }
      Logger& operator<<(const std::string& t) {
        if (lseek(logfd_, 0, SEEK_CUR) >= max_size_) {
          ftruncate(logfd_, 0);
          lseek(logfd_, 0, SEEK_SET);
        }
        write(logfd_, t.c_str(), sizeof(char) * t.length());
        return *this;
      }
      Logger& operator<<(const char* t) {
        if (lseek(logfd_, 0, SEEK_CUR) >= max_size_) {
          ftruncate(logfd_, 0);
          lseek(logfd_, 0, SEEK_SET);
        }
        write(logfd_, t, strlen(t));
        return *this;
      }
      template<typename T>
      Logger& operator<<(const T& t) {
        if (lseek(logfd_, 0, SEEK_CUR) >= max_size_) {
          ftruncate(logfd_, 0);
          lseek(logfd_, 0, SEEK_SET);
        }
        std::string str = std::to_string(t);
        write(logfd_, str.c_str(), sizeof(char) * str.length());
        return *this;
      }
      void Close() {
        close(logfd_);
      }
    private:
      int logfd_;
      int max_size_;
};

} // namespace WebServer

#endif