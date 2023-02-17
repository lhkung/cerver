#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <memory>
#include <unordered_map>
#include <vector>
#include <pthread.h>
#include "server.h"
#include "threadpool.h"
#include "tcpconnection.h"
#include "logger.h"
#include "lru_cache.h"

namespace Cerver {

class HttpRequest {
public:
  HttpRequest() { }
  virtual ~HttpRequest() { }
  void AddHeader(const std::string& name, const std::string& value) {
    headers_.insert({name, value});
  }
  std::string method_;
  std::string uri_;
  std::string protocol_;
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
};

class HttpResponse {
public:
  HttpResponse() : has_body_(false), has_file_(false){ }
  virtual ~HttpResponse() { }
  void AddHeader(const std::string& name, const std::string& value) {
    headers_.insert({name, value});
  }
  std::string protocol_;
  int status_code_;
  std::string reason_phrase_;
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
  std::string file_;
  std::string uri_;
  size_t file_size_;
  bool has_body_;
  bool has_file_;
};

class HttpServer : public Server {

public:
  HttpServer(int max_thread, int listen_port, int cache_size, std::string dir, std::string logdir);
  virtual ~HttpServer();
  void Run() override;
  void ThreadLoop(int comm_fd);
  bool ParseRequest(std::string& header, HttpRequest* req);
  void ProcessRequest(const HttpRequest& req, HttpResponse* res);
  void SendResponse(const HttpResponse& res, TCPConnection* conn);
  void SetErrCode(int status_code, HttpResponse* res);
  int SendFile(const HttpResponse& res, TCPConnection* conn);
  std::string GetContentType(const std::string& path);
  void PrintStat();

  struct Stat {
    int num_conn_;
    int num_read_;
    int num_cache_;
    size_t byte_read_;
    size_t byte_cache_;
    int incomplete_cache_entry_;
    Stat() : num_conn_(0), num_read_(0), num_cache_(0), byte_read_(0), byte_cache_(0), incomplete_cache_entry_(0) { }
  };

private:

  std::unique_ptr<ThreadPool> threadpool_;
  std::unique_ptr<Logger> log_;
  std::unique_ptr<LRUCache> cache_;
  std::string dir_;
  int listen_port_;
  pthread_mutex_t loglock_;
  Stat stat_;
};

class HttpServerTask : public ThreadPool::Task {

public:
  explicit HttpServerTask(int comm_fd, HttpServer* server);
  virtual ~HttpServerTask();
  void Run() override;
  int comm_fd_;
  HttpServer* server_;
};

static void LowerCase(std::string& str);
static void Trim(std::string& str);
static int Split(std::string& str, const std::string& delim, std::vector<std::string>* out);
static bool IsNumber(const std::string& str);
static const char* GetTime();

} // end namespace WebServer


#endif

