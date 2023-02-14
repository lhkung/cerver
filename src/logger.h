#ifndef LOGGER_H_
#define LOGGER_H_
#include <string>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <pthread.h>

namespace Cerver {

class Logger {
    public:
      Logger() : init_(false), logfd_(-1) {}
      Logger(std::string dir) : init_(true) {
        std::string path = dir + "/run.log";
        logfd_ = open(path.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
        if (logfd_ == -1) {
          std::cout << errno << std::endl;
        }
        pthread_mutex_init(&lock_, nullptr);
      }
      virtual ~Logger() { 
        if (init_) {
          //close(logfd_);
          // pthread_mutex_destroy(&lock_);
        }
      }
      Logger& operator<<(const std::string& t) {
        pthread_mutex_lock(&lock_);
        write(logfd_, t.c_str(), sizeof(char) * t.length());
        pthread_mutex_unlock(&lock_);
        return *this;
      }
      Logger& operator<<(const char* t) {
        pthread_mutex_lock(&lock_);
        write(logfd_, t, strlen(t));
        pthread_mutex_unlock(&lock_);
        return *this;
      }
      template<typename T>
      Logger& operator<<(const T& t) {
        std::string str = std::to_string(t);
        pthread_mutex_lock(&lock_);
        write(logfd_, str.c_str(), sizeof(char) * str.length());
        pthread_mutex_unlock(&lock_);
        return *this;
      }
    private:
      bool init_;
      int logfd_;
      pthread_mutex_t lock_;
};

} // namespace WebServer

#endif