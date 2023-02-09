#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <memory>
#include <unordered_map>
#include <vector>
#include "server.h"
#include "threadpool.h"
#include "tcpconnection.h"

namespace WebServer {

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
  HttpResponse() { }
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
  bool has_body_;
  bool has_file_;
};

class HttpServer : public Server {

public:
  HttpServer();
  HttpServer(int max_thread, int listen_port, const std::string& directory);
  virtual ~HttpServer();
  void Run() override;
  void ThreadLoop(int comm_fd);
  bool ParseRequest(std::string& header, HttpRequest* req);
  void ProcessRequest(const HttpRequest& req, HttpResponse* res);
  void SendResponse(const HttpResponse& res, TCPConnection* conn);
  void SetErrCode(int status_code, HttpResponse* res);
  int SendFile(int file_fd, TCPConnection* conn);
  std::string GetContentType(const std::string& path);

private:
  std::unique_ptr<ThreadPool> threadpool_;
  int listen_port_;
  std::string directory_;
};

class HttpServerTask : public ThreadPool::Task {

public:
  explicit HttpServerTask(int comm_fd, HttpServer* server);
  virtual ~HttpServerTask();
  void Run() override;
  int comm_fd_;
  HttpServer* server_;
};

} // end namespace WebServer


#endif

