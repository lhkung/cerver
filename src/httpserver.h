#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#define DATA_BATCH 4194304 // 4 MB

#include <memory>
#include <unordered_map>
#include <vector>
#include <pthread.h>
#include "server.h"
#include "threadpool.h"
#include "tcpconnection.h"
#include "logger.h"
#include "lrucache.h"
#include "httprequest.h"
#include "httpresponse.h"

namespace Cerver {

class HttpServer : public Server {

public:
  typedef std::function<std::string(const HttpRequest&, HttpResponse*)> Route;
  HttpServer(int max_thread, int listen_port);
  virtual ~HttpServer();
  void Run() override;
  void ThreadLoop(int comm_fd);
  int PrepareRequest(const std::string& header, HttpRequest* req, TCPConnection* conn, Route** route);
  void SendResponse(HttpResponse* res, TCPConnection* conn, const std::string& body);
  void PrintStat();
  void GetStats(const HttpRequest& req, HttpResponse* res);
  Route* CollectPathParam(HttpRequest* req);
  void CollectQueryParam(HttpRequest* req);
  void Put(const std::string& route, Route lambda);
  void Get(const std::string& route, Route lambda);

  static void SetErrCode(int status_code, HttpResponse* res);
  static void ReadFile(HttpResponse* res, const std::string& path);
  static std::string GetContentType(const std::string& path);

  class Stats {
    public:
      Stats();
      ~Stats();
      void IncConn();
      void DecConn();
      void IncReq();
      int GetConn() const;
      int GetReq() const;
    private:
      uint32_t num_conn_;
      uint32_t num_req_;
      pthread_mutex_t lock_;
  };

private:
  std::unique_ptr<ThreadPool> threadpool_;
  std::unique_ptr<Logger> log_;
  int listen_port_;
  Stats stat_;
  std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<Route> > > routes_;
};

class HttpServerTask : public ThreadPool::Task {

public:
  explicit HttpServerTask(int comm_fd, HttpServer* server);
  virtual ~HttpServerTask();
  void Run() override;
  int comm_fd_;
  HttpServer* server_;
};

static std::unique_ptr<HttpServer> server;

} // end namespace Cerver

#endif

