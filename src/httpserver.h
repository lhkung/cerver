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
#include "lrucache.h"
#include "httprequest.h"
#include "httpresponse.h"

namespace Cerver {

class HttpServer : public Server {

public:
  typedef std::function<std::string(const HttpRequest&, HttpResponse*)> Route;
  HttpServer(int max_thread, int listen_port, int cache_size, std::string dir, std::string logdir);
  virtual ~HttpServer();
  void Run() override;
  void ThreadLoop(int comm_fd);
  int PrepareRequest(const std::string& header, HttpRequest* req, TCPConnection* conn, Route** route);
  void NoRoute(const HttpRequest& req, HttpResponse* res);
  void SendResponse(HttpResponse* res, TCPConnection* conn, const std::string& body);
  void SetErrCode(int status_code, HttpResponse* res);
  int SendFile(const HttpResponse& res, TCPConnection* conn);
  std::string GetContentType(const std::string& path);
  void PrintStat();
  void GetStats(const HttpRequest& req, HttpResponse* res);
  Route* CollectPathParam(HttpRequest* req);
  void CollectQueryParam(HttpRequest* req);
  void Put(const std::string& route, Route lambda);
  void Get(const std::string& route, Route lambda);

  class Stats {
    public:
      Stats();
      ~Stats();
      void IncConn();
      void DecConn();
      void IncRead();
      void IncCache();
      void IncByteRead(const size_t b);
      void IncByteCache(const size_t b);
      int GetConn() const;
      int GetRead() const;
      int GetCache() const;
      size_t GetByteRead() const;
      size_t GetByteCache() const;
    private:
      int num_conn_;
      int num_read_;
      int num_cache_;
      size_t byte_read_;
      size_t byte_cache_;
      pthread_mutex_t lock_;
  };


private:
  std::unique_ptr<ThreadPool> threadpool_;
  std::unique_ptr<Logger> log_;
  std::unique_ptr<LRUStringCache> cache_;
  std::string dir_;
  int listen_port_;
  pthread_mutex_t loglock_;
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

} // end namespace WebServer


#endif

