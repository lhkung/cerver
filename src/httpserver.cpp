#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "httpserver.h"

using std::string;
using std::unique_ptr;
using std::vector;
using std::unordered_map;

namespace WebServer {

static void HandleSigint(int signum) {

}

static unordered_map<int, string> err_codes = {{404, "Not Found"},
                                               {405, "Method Not Allowed"},
                                               {505, "HTTP Version not supported"}};

HttpServer::HttpServer()
  : threadpool_(std::make_unique<ThreadPool>(100)), listen_port_(8000) { }

HttpServer::HttpServer(int max_thread, int listen_port, const string& directory)
  : threadpool_(std::make_unique<ThreadPool>(max_thread)), listen_port_(listen_port), directory_(directory) { }

HttpServer::~HttpServer() { }

void HttpServer::Run() {
  PrepareToHandleSignal(SIGINT, HandleSigint);
  int listen_fd = CreateListenSocket(listen_port_, 100);
  if (listen_fd == -1) {
    std::cerr << "Failed to create listening socket. " << strerror(errno) << std::endl;
    return;
  }
  while (true) {
    string addr;
    int port;
    int comm_fd = AcceptConnection(listen_fd, &addr, &port);
    if (comm_fd == -1) {
      break;
    }
    std::cout << "Connection from " << addr << ":" << port << std::endl;
    unique_ptr<ThreadPool::Task> task = std::make_unique<HttpServerTask>(comm_fd, this);
    threadpool_->Dispatch(std::move(task));
  }
  threadpool_->KillThreads();
  std::cerr << "Server shut down." << std::endl;

}

void HttpServer::ThreadLoop(int comm_fd) {
  TCPConnection conn = TCPConnection(comm_fd);
  while (true) {
    string header;
    int ret = conn.ReadUntilDoubleCRLF(&header);
    if (ret <= 0) {
      break;
    }
    HttpRequest req;
    bool req_valid = ParseRequest(header, &req);
    if (!req_valid) {
      continue;
    }
    auto it = req.headers_.find("content-length");
    if (it != req.headers_.end()) {
      int len = stoi(it->second);
      conn.ReadSize(len, &(req.body_));
      if (ret <= 0) {
        break;
      }
    }
    HttpResponse res;
    ProcessRequest(req, &res);
    SendResponse(res, &conn);
  }
  conn.Close();
  std::cerr << "Connection closed" << std::endl;
}

bool HttpServer::ParseRequest(string& header, HttpRequest* req) {
  vector<string> lines;
  int num_lines = Split(header, "\r\n", &lines);
  if (num_lines < 1) {
    return false;
  }
  vector<string> first_line;
  int num_tok = Split(lines[0], " ", &first_line);
  if (num_tok < 3) {
    return false;
  }
  LowerCase(first_line[0]);
  LowerCase(first_line[2]);
  req->method_ = first_line[0];
  req->uri_ = first_line[1];
  req->protocol_ = first_line[2];
  for (int i = 1; i < lines.size(); i++) {
    vector<string> kv;
    Split(lines[i], ":", &kv);
    LowerCase(kv[0]);
    Trim(kv[0]);
    Trim(kv[1]);
    req->AddHeader(kv[0], kv[1]);
  }
  return true;
}

void HttpServer::ProcessRequest(const HttpRequest& req, HttpResponse* res) {
  if (req.method_ != "get") {
    SetErrCode(405, res);
    return;
  }
  if (req.uri_ != "/") {
    SetErrCode(404, res);
    return;
  }
  if (req.protocol_ != "http/1.1") {
    SetErrCode(505, res);
    return;
  }
  res->protocol_ = "HTTP/1.1";
  res->status_code_ = 200;
  res->reason_phrase_ = "OK";
  res->AddHeader("Content-Type", "text/html");
  res->has_file_ = true;
  string path = directory_ + "/test.html";
  int file = open(path.c_str(), O_RDONLY);
  if (file == -1) {
    std::cout << strerror(errno) << std::endl;
    SetErrCode(404, res);
    return;
  }
  off_t size = lseek(file, 0, SEEK_END);
  close(file);
  res->file_ = path;
  res->AddHeader("Content-Length", std::to_string(size));
}

void HttpServer::SendResponse(const HttpResponse& res, TCPConnection* conn) {
  conn->Send(res.protocol_ + " " + std::to_string(res.status_code_) + " " + res.reason_phrase_ + "\r\n");
  for (auto it = res.headers_.begin(); it != res.headers_.end(); it++) {
    conn->Send(it->first + ": " + it->second + "\r\n");
  }
  conn->Send("\r\n");
  if (res.has_body_) {
    conn->Send(res.body_);
    return;
  }
  if (res.has_file_) {
    int file = open(res.file_.c_str(), O_RDONLY);
    SendFile(file, conn);
    close(file);
  }
}

void HttpServer::SetErrCode(int err_code, HttpResponse* res) {
  res->protocol_ = "HTTP/1.1";
  auto it = err_codes.find(err_code);
  if (it == err_codes.end()) {
    res->status_code_ = 404;
    res->reason_phrase_ = err_codes.at(404);
    res->AddHeader("Content-Type", "text/plain");
    res->AddHeader("Content-Length", std::to_string(res->reason_phrase_.length()));
    res->has_body_ = true;
    res->body_ = res->reason_phrase_;
    return;
  }
  res->status_code_ = err_code;
  res->reason_phrase_ = err_codes.at(err_code);
  res->AddHeader("Content-Type", "text/plain");
  res->AddHeader("Content-Length", std::to_string(res->reason_phrase_.length()));
  res->has_body_ = true;
  res->body_ = res->reason_phrase_;
}

int HttpServer::Split(string& str, const string& delim, vector<string>* out) {
  size_t index = str.find(delim);
  while (index != string::npos) {
    out->push_back(str.substr(0, index));
    str = str.substr(index + delim.length());
    index = str.find(delim);
  }
  if (str.length() != 0) {
    out->push_back(str);
  }
  return out->size();
}

void HttpServer::LowerCase(string& str) {
  for (int i = 0; i < str.length(); i++) {
    if (str[i] >= 'A' && str[i] <= 'Z') {
      str[i] = static_cast<char>(str[i] + 32);
    }
  }
}

void HttpServer::Trim(string& str) {
  int start = 0;
  int end = str.length() - 1;
  while (str[start] == ' ') {
    start++;
  }
  while (str[end] == ' ') {
    end--;
  }
  str = str.substr(start, end + 1);
}

int HttpServer::SendFile(int file_fd, TCPConnection* conn) {
  if (file_fd == -1) {
    return -1;
  }
  char buf[4096];
  int total_bytes = 0;
  ssize_t bytes_read = read(file_fd, buf, 4096);
  while (bytes_read > 0) {
    conn->Send(string(buf, bytes_read));
    total_bytes += bytes_read;
    bytes_read = read(file_fd, buf, 4096);
  }
  return total_bytes;
}

HttpServerTask::HttpServerTask(int comm_fd, HttpServer* server) 
  : comm_fd_(comm_fd), server_(server) { }
HttpServerTask::~HttpServerTask() { }
void HttpServerTask::Run() {
  server_->ThreadLoop(comm_fd_);
}

} // end namespace WebServer

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "./httpserver <directory>" << std::endl;
    return 1;
  }
  unique_ptr<WebServer::Server> server = std::make_unique<WebServer::HttpServer>(32, 8000, string(argv[1]));
  server->Run();
  return 0;
}