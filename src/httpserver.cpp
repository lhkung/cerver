#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "httpserver.h"
#include "utils.h"

#define DATA_BATCH 4194304 // 4 MB
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

HttpServer::Stats::Stats() : num_conn_(0), num_read_(0), num_cache_(0), byte_read_(0), byte_cache_(0) {
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
void HttpServer::Stats::IncRead() {
  pthread_mutex_lock(&lock_);
  num_read_++;
  pthread_mutex_unlock(&lock_);
}
void HttpServer::Stats::IncCache() {
  pthread_mutex_lock(&lock_);
  num_cache_++;
  pthread_mutex_unlock(&lock_);
}
void HttpServer::Stats::IncByteRead(const size_t b) {
  pthread_mutex_lock(&lock_);
  byte_read_ += b;
  pthread_mutex_unlock(&lock_);
}
void HttpServer::Stats::IncByteCache(const size_t b) {
  pthread_mutex_lock(&lock_);
  byte_cache_ += b;
  pthread_mutex_unlock(&lock_);
}
int HttpServer::Stats::GetConn() const {return num_conn_;}
int HttpServer::Stats::GetRead() const {return num_read_;}
int HttpServer::Stats::GetCache() const {return num_cache_;}
size_t HttpServer::Stats::GetByteRead() const {return byte_read_;}
size_t HttpServer::Stats::GetByteCache() const {return byte_cache_;}


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

HttpServer::HttpServer(int max_thread, int listen_port, int cache_size, string dir, string logdir)
  : threadpool_(std::make_unique<ThreadPool>(max_thread)),
    log_(std::make_unique<Logger>(logdir, 1024 * 1024 * 1024)),
    cache_(std::make_unique<LRUStringCache>(cache_size)),
    dir_(dir),
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
    *log_ << "Attempt to read from socket\n"; 
    int ret = conn.ReadUntilDoubleCRLF(&header);
    if (ret <= 0) {
      *log_ << "Read interrupted\n"; 
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
    if (req_status == REQ_INVALID) {
      SetErrCode(404, &res);
      SendResponse(&res, &conn, res.Body());
      continue;
    }
    if (req_status == NO_ROUTE) {
      NoRoute(req, &res);
      SendResponse(&res, &conn, res.Body());
      continue;
    }
    string out = (*route)(req, &res);
    if (res.Written()) {
      *log_ << GetTime() << "This route has written to client\n";
      conn.Close();
      return;
    }
    if (out.length() > 0) {
      *log_ << GetTime() << "Send lambda's return value to client\n";
      SendResponse(&res, &conn, out);
      continue;
    }
    if (res.HasBody()) {
      *log_ << GetTime() << "Send response body to client\n";
      SendResponse(&res, &conn, res.Body());
      continue;
    }
    *log_ << GetTime() << "Send only header to client\n";
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

void HttpServer::NoRoute(const HttpRequest& req, HttpResponse* res) {
  if (req.Method() != "get") {
    SetErrCode(405, res);
    return;
  }
  if (req.protocol_ != "http/1.1") {
    SetErrCode(505, res);
    return;
  }
  string path;
  if (req.uri_ == "/") {
     path = dir_ + "/index.html";
  } else {
    path = dir_ + "/" + req.uri_;
  }
  int file = open(path.c_str(), O_RDONLY);
  if (file == -1) {
    *log_ << path << " not found.\n"; 
    SetErrCode(404, res);
    return;
  }
  res->SetProtocol("HTTP/1.1");
  res->SetStatusCode(200, "OK");
  res->PutHeader("Content-Type", GetContentType(path));
  res->has_file_ = true;
  off_t size = lseek(file, 0, SEEK_END);
  close(file);
  res->file_size_ = static_cast<size_t>(size);
  res->file_ = path;
  res->uri_ = req.uri_;
  res->PutHeader("Content-Length", std::to_string(size));
}

void HttpServer::SendResponse(HttpResponse* res, TCPConnection* conn, const string& body) {
  *log_ << GetTime() << "Sending response header " << res->StatusCode() << "\n";
  if (body.length() > 0) {
      res->PutHeader("Content-Length", std::to_string(body.length()));
  }
  conn->Send("HTTP/1.1 " + std::to_string(res->StatusCode()) + " " + res->Reason() + "\r\n");
  for (auto it = res->headers_.begin(); it != res->headers_.end(); it++) {
    conn->Send(it->first + ": " + it->second + "\r\n");
  }
  conn->Send("\r\n");
  if (body.length() > 0) {
    conn->Send(body);
  }
  if (res->has_file_) {
    SendFile(*res, conn);
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
    res->PutHeader("Content-Length", std::to_string(res->reason_phrase_.length()));
    res->has_body_ = true;
    res->SetBody(res->reason_phrase_);
    return;
  }
  res->SetStatusCode(err_code, it->second);
  res->PutHeader("Content-Type", "text/plain");
  res->PutHeader("Content-Length", std::to_string(res->reason_phrase_.length()));
  res->has_body_ = true;
  res->SetBody(res->reason_phrase_);
}

int HttpServer::SendFile(const HttpResponse& res, TCPConnection* conn) {
  string val;
  if (cache_->Get(res.uri_, &val) == 0) {
    if (val.length() == res.file_size_) {
      conn->Send(val);
      stat_.IncCache();
      stat_.IncRead();
      stat_.IncByteCache(val.length());
      stat_.IncByteRead(val.length());
      return val.length();
    }
  }
  int file_fd = open(res.file_.c_str(), O_RDONLY);
  char* buf = new char[DATA_BATCH];
  size_t total_bytes = 0;
  ssize_t bytes_read = read(file_fd, buf, DATA_BATCH);
  while (bytes_read > 0) {
    conn->Send(string(buf, bytes_read));
    total_bytes += bytes_read;
    bytes_read = read(file_fd, buf, DATA_BATCH);
  }
  if (total_bytes <= cache_->Capacity()) {
    cache_->Put(res.uri_, string(buf, total_bytes));
  }
  delete[] buf;
  stat_.IncRead();
  stat_.IncByteRead(total_bytes);
  return total_bytes;
}

void HttpServer::GetStats(const HttpRequest& req, HttpResponse* res) {
  res->SetProtocol("HTTP/1.1");
  res->SetStatusCode(200, "OK");
  res->PutHeader("Content-Type", "text/html");
  res->has_body_ = true;
  string cache_keys;
  cache_->GetKeys(&cache_keys);

  char* body = new char[4096];
  int len = sprintf(body, "<html><h2>HttpServer status</h2><body><p>Listening on port %d<br>Number of threads: %d<br>Number of active connections: %d<br>Number of tasks in work queue: %ld<br>Cache size: %ld  / %ld bytes<br>Cache entries: %d<br>%s<br>Cache hit rate (time): %d / %d = %.2f<br>Cache hit rate (byte): %ld / %ld = %.2f</p></body></html>",
                          listen_port_,
                          threadpool_->num_threads_running_,
                          stat_.GetConn(),
                          threadpool_->work_queue_.size(),
                          cache_->Size(),
                          cache_->Capacity(),
                          cache_->Entry(),
                          cache_keys.c_str(),
                          stat_.GetCache(),
                          stat_.GetRead(),
                          static_cast<float>(stat_.GetCache()) / static_cast<float>(stat_.GetRead()),
                          stat_.GetByteCache(),
                          stat_.GetByteRead(),
                          static_cast<float>(stat_.GetByteCache()) / static_cast<float>(stat_.GetByteRead()));
  res->PutHeader("Content-Length", std::to_string(len));
  res->body_ = string(body, len);
  delete[] body;
}

void HttpServer::PrintStat() {
  std::cout << "HttpServer status\n";
  std::cout << "Listening on port " << listen_port_ << "\n";
  std::cout << "Number of threads: " << threadpool_->num_threads_running_ << "\n";
  std::cout << "Number of active connections: " << stat_.GetConn() << "\n";
  std::cout << "Number of tasks in work queue: " << threadpool_->work_queue_.size() << "\n";
  std::cout << "Cache size: " << cache_->Size() << "\n";
  std::cout << "Cache entries: " << cache_->Entry() << "\n";
  std::cout << "Cache hit rate (time): " << stat_.GetCache() << "/" << stat_.GetRead() << " = " <<  static_cast<float>(stat_.GetCache()) / static_cast<float>(stat_.GetRead()) << "\n";
  std::cout << "Cache hit rate (byte): " << stat_.GetByteCache() << "/" << stat_.GetByteRead() << " = " <<  static_cast<float>(stat_.GetByteCache()) / static_cast<float>(stat_.GetByteRead()) << std::endl;
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
  auto it = routes_.find("put");
  if (it == routes_.end()) {
    routes_.emplace("put", unordered_map<string, unique_ptr<Route>>());
  } 
  routes_.at("put").emplace(route, std::make_unique<Route>(lambda));
}
void HttpServer::Get(const string& route, Route lambda) {
  auto it = routes_.find("get");
  if (it == routes_.end()) {
    routes_.emplace("get", unordered_map<string, unique_ptr<Route>>());
  } 
  routes_.at("get").emplace(route, std::make_unique<Route>(lambda));
}

} // end namespace WebServer