#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "httpserver.h"
#include "utils.h"

#define REQ_INVALID 1
#define ROUTE_FOUND 2
#define NO_ROUTE 3

using std::string;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::unordered_map;

namespace Cerver {

volatile bool running = true;
volatile bool stat = false;

HttpServer::Stats::Stats() : num_conn_(0), num_req_(0) {
  pthread_mutex_init(&lock_, nullptr);
}
HttpServer::Stats::~Stats() {
  pthread_mutex_destroy(&lock_);
}
void HttpServer::Stats::IncConn() {
  pthread_mutex_lock(&lock_);
  num_conn_++;
  pthread_mutex_unlock(&lock_);
}
void HttpServer::Stats::DecConn() {
  pthread_mutex_lock(&lock_);
  num_conn_--;
  pthread_mutex_unlock(&lock_);
}
void HttpServer::Stats::IncReq() {
  pthread_mutex_lock(&lock_);
  num_req_++;
  pthread_mutex_unlock(&lock_);
}
int HttpServer::Stats::GetConn() const {return num_conn_;}
int HttpServer::Stats::GetReq() const {return num_req_;}

static void HandleSignal(int signum) {
  if (signum == SIGINT) {
    running = false;
  } else if (signum == SIGUSR1) {
    stat = true;
  }
}

static unordered_map<int, string> err_codes = {{404, "Not Found"},
                                               {405, "Method Not Allowed"},
                                               {505, "HTTP Version not supported"}};

HttpServer::HttpServer(int max_thread, int listen_port)
  : threadpool_(std::make_unique<ThreadPool>(max_thread)),
    log_(std::make_unique<Logger>("cerverlog", 1024 * 1024 * 1024)),
    listen_port_(listen_port),
    stat_() 
{
    pthread_mutex_init(&loglock_, nullptr);
    Get("/stats", [this](const HttpRequest& req, HttpResponse* res) {
      GetStats(req, res);
      return "";
    });
}

HttpServer::~HttpServer() {
  pthread_mutex_destroy(&loglock_);
}

void HttpServer::Run() {
  PrepareToHandleSignal(SIGINT, HandleSignal);
  PrepareToHandleSignal(SIGUSR1, HandleSignal);
  *log_ << GetTime() << "Server starts\n";
  int listen_fd = CreateListenSocket(listen_port_, 100);
  if (listen_fd == -1) {
    std::cout << "Failed to create listen socket" << std::endl;
    return;
  }
  while (true) {
    string addr;
    int port;
    int comm_fd = AcceptConnection(listen_fd, &addr, &port);
    if (comm_fd == -1) {
      if (errno == EINTR) {
        if (stat) {
          PrintStat();
        }
        if (!running) {
          break;
        }
      }
      continue;
    }
    stat_.IncConn();
    *log_ << GetTime() << "Connection from " << addr << ":" << port << "\n";
    unique_ptr<ThreadPool::Task> task = std::make_unique<HttpServerTask>(comm_fd, this);
    threadpool_->Dispatch(std::move(task));
  }
  threadpool_->KillThreads();
  *log_ << GetTime() << "Server shut down\n";
  std::cout << "Server shut down" << std::endl;
}


HttpServerTask::HttpServerTask(int comm_fd, HttpServer* server) 
  : comm_fd_(comm_fd), server_(server) { }
HttpServerTask::~HttpServerTask() { }
void HttpServerTask::Run() {
  server_->ThreadLoop(comm_fd_);
}

void HttpServer::ThreadLoop(int comm_fd) {
  TCPConnection conn = TCPConnection(comm_fd);
  while (true) {
    string header;
    int ret = conn.ReadUntilDoubleCRLF(&header);
    if (ret <= 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    HttpRequest req;
    HttpResponse res;
    Route* route = nullptr;
    int req_status = PrepareRequest(header, &req, &conn, &route);
    *log_ << GetTime() << req.Method() << " " << req.URI() << " " << req.Protocol() << "\n";
    stat_.IncReq();
    if (req_status == REQ_INVALID) {
      SetErrCode(404, &res);
      SendResponse(&res, &conn, res.Body());
      continue;
    }
    if (req_status == NO_ROUTE) {
      SetErrCode(404, &res);
      SendResponse(&res, &conn, res.Body());
      continue;
    }
    string out = (*route)(req, &res);
    if (res.Written()) {
      conn.Close();
      return;
    }
    if (out.length() > 0) {
      SendResponse(&res, &conn, out);
      continue;
    }
    if (res.HasBody()) {
      SendResponse(&res, &conn, res.Body());
      continue;
    }
    SendResponse(&res, &conn, "");
  }
  conn.Close();
  stat_.DecConn();
  *log_ << GetTime() << "Connection closed\n";
}

int HttpServer::PrepareRequest(const string& header, HttpRequest* req, TCPConnection* conn, Route** route) {
  vector<string> lines;
  int num_lines = Split(header, "\r\n", &lines);
  if (num_lines < 1) {
    return REQ_INVALID;
  }
  vector<string> first_line;
  int num_tok = Split(lines[0], " ", &first_line);
  if (num_tok < 3) {
    return REQ_INVALID;
  }
  LowerCase(first_line[0]);
  LowerCase(first_line[2]);
  req->SetMethod(first_line[0]);
  req->SetURI(first_line[1]);
  req->SetProtocol(first_line[2]);
  if (routes_.find(req->Method()) == routes_.end()) {
    return NO_ROUTE;
  }
  Route* r = CollectPathParam(req);
  CollectQueryParam(req);
  for (uint i = 1; i < lines.size(); i++) {
    vector<string> kv;
    int numElement = Split(lines[i], ":", &kv);
    if (numElement < 2) {
      continue;
    }
    LowerCase(kv[0]);
    Trim(kv[0]);
    Trim(kv[1]);
    req->PutHeader(kv[0], kv[1]);
  }
  int len = req->ContentLength();
  if (len > 0) {
    conn->ReadSize(len, &(req->body_));
  }
  if (r == nullptr) {
    return NO_ROUTE;
  }
  *route = r;
  return ROUTE_FOUND;
}

void HttpServer::SendResponse(HttpResponse* res, TCPConnection* conn, const string& body) {
  *log_ << GetTime() << "Sending response header " << res->StatusCode() << "\n";
  if (body.length() > 0) {
      res->PutHeader("Content-Length", std::to_string(body.length()));
  }
  conn->Send("HTTP/1.1 " + std::to_string(res->StatusCode()) + " " + res->Reason() + "\r\n");
  for (auto it = res->Headers().begin(); it != res->Headers().end(); it++) {
    conn->Send(it->first + ": " + it->second + "\r\n");
  }
  conn->Send("\r\n");
  if (body.length() > 0) {
    conn->Send(body);
  }
  *log_ << GetTime() << "Response sent\n";
}

string HttpServer::GetContentType(const string& path) {
  string ext = ""; 
  for (uint i = path.length() - 1; i >= 0; i--) {
    ext += path[i];
    if (path[i] == '.') {
      break;
    }
  }
  int l = 0;
  int r = ext.length() - 1;
  while (l < r) {
    char temp = ext[l];
    ext[l] = ext[r];
    ext[r] = temp;
    l++;
    r--;
  }
  if (ext == ".jpeg") {
    return "image/jpeg";
  }
  if (ext == ".png") {
    return "image/png";
  }
  if (ext == ".jpg") {
    return "image/jpg";
  }
  if (ext == ".html") {
    return "text/html";
  }
  if (ext == ".css") {
    return "text/css";
  }
  return "text/plain";
}

void HttpServer::SetErrCode(int err_code, HttpResponse* res) {
  res->SetProtocol("HTTP/1.1");
  auto it = err_codes.find(err_code);
  if (it == err_codes.end()) {
    res->SetStatusCode(404, err_codes.at(404));
    res->PutHeader("Content-Type", "text/plain");
    res->PutHeader("Content-Length", std::to_string(res->Reason().length()));
    res->SetBody(res->Reason());
    return;
  }
  res->SetStatusCode(err_code, it->second);
  res->PutHeader("Content-Type", "text/plain");
  res->PutHeader("Content-Length", std::to_string(res->Reason().length()));
  res->SetBody(res->Reason());
}

void HttpServer::GetStats(const HttpRequest& req, HttpResponse* res) {
  res->SetProtocol("HTTP/1.1");
  res->SetStatusCode(200, "OK");
  res->PutHeader("Content-Type", "text/html");

  char* body = new char[4096];
  int len = sprintf(body, "<html><h2>HttpServer status</h2><body><p>Listening on port %d<br>Number of threads: %d<br>Number of active connections: %d<br>Number of tasks in work queue: %ld<br>Accumulative number of requests: %d</p></body></html>",
                          listen_port_,
                          threadpool_->num_threads_running_,
                          stat_.GetConn(),
                          threadpool_->work_queue_.size(),
                          stat_.GetReq());
  res->PutHeader("Content-Length", std::to_string(len));
  res->SetBody(string(body, len));
  delete[] body;
}

void HttpServer::PrintStat() {
  std::cout << "HttpServer status\n";
  std::cout << "Listening on port " << listen_port_ << "\n";
  std::cout << "Number of threads: " << threadpool_->num_threads_running_ << "\n";
  std::cout << "Number of active connections: " << stat_.GetConn() << "\n";
  std::cout << "Number of tasks in work queue: " << threadpool_->work_queue_.size() << std::endl;
  stat = false;
}

HttpServer::Route* HttpServer::CollectPathParam(HttpRequest* req)  {
  std::string uri_parse = req->URI();
  int question_mark = uri_parse.find("?");
  if (question_mark != string::npos) {
    uri_parse = uri_parse.substr(0, question_mark);
  }
  vector<string> uri_split;
  Split(uri_parse, "/", &uri_split);
  const unordered_map<string, std::unique_ptr<Route> >& paths = routes_.at(req->Method());
  for (auto it = paths.begin(); it != paths.end(); it++) {
    bool match = true;
    vector<string> path_split;
    Split(it->first, "/", &path_split);
    if (path_split.size() != uri_split.size()) {
      continue;
    }
    for (int i = 0; i < path_split.size(); i++) {
      if (path_split[i] == uri_split[i]) {
        continue;
      }
      if (path_split[i][0] == ':') {
        req->PutPathParam(path_split[i].substr(1), uri_split[i]);
        continue;
      }
      match = false;
      break;
    }
    if (match) {
      return it->second.get();
    }
		req->ClearPathParam();
  }
  return nullptr;
}

void HttpServer::CollectQueryParam(HttpRequest* req) {
  int questionMark = req->URI().find("?");
		if (questionMark == string::npos) {
			return;
		}
    vector<string> queries;
    Split(req->URI().substr(questionMark + 1), "&", &queries);

		for (string pair : queries) {
      vector<string> kv;
			int numElements = Split(pair, "=", &kv);
      if (numElements == 0) {
        continue;
      }
			if (numElements == 1) {
				req->PutQueryParam(kv[0], "");
			} else {
				req->PutQueryParam(kv[0], kv[1]);
			}
		}
		return;
}

void HttpServer::Put(const string& route, Route lambda) {
  // if (server == nullptr) {
  //   server = std::make_unique<HttpServer>(32, 80);
  //   server->Run();
  // }
  auto it = routes_.find("put");
  if (it == routes_.end()) {
    routes_.emplace("put", unordered_map<string, unique_ptr<Route>>());
  } 
  routes_.at("put").emplace(route, std::make_unique<Route>(lambda));
}
void HttpServer::Get(const string& route, Route lambda) {
  // if (server == nullptr) {
  //   server = std::make_unique<HttpServer>(32, 80);
  //   server->Run();
  // }
  auto it = routes_.find("get");
  if (it == routes_.end()) {
    routes_.emplace("get", unordered_map<string, unique_ptr<Route>>());
  } 
  routes_.at("get").emplace(route, std::make_unique<Route>(lambda));
}

void HttpServer::ReadFile(const HttpRequest& req, HttpResponse* res, const string& path) {
  int file_fd = open(path.c_str(), O_RDONLY);
  if (file_fd == -1) {
    HttpServer::SetErrCode(404, res);
    return;
  }
  char* buf = new char[DATA_BATCH];
  size_t total_bytes = 0;
  while (true) {
    ssize_t bytes_read = read(file_fd, buf, DATA_BATCH);
    if (bytes_read <= 0) {
       break;
    }
    res->AppendBody(string(buf, bytes_read));
    total_bytes += bytes_read;
  }
  res->SetProtocol("HTTP/1.1");
  res->SetStatusCode(200, "OK");
  res->PutHeader("Content-Type", HttpServer::GetContentType(path));
  res->PutHeader("Content-Length", std::to_string(total_bytes));
  delete[] buf;
}

} // end namespace WebServer