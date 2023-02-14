#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "httpserver.h"

#define DATA_BATCH 4194304 // 4 MB

using std::string;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::unordered_map;

static string DIR;

namespace WebServer {

static void LowerCase(string& str);
static void Trim(string& str);
static int Split(string& str, const string& delim, vector<string>* out);
static bool IsNumber(const string& str);
static void HandleSigint(int signum);
static bool ParseRequest(string& header, HttpRequest* req);
static void ThreadLoop(int comm_fd);
static bool ParseRequest(string& header, HttpRequest* req);
static void ProcessRequest(const HttpRequest& req, HttpResponse* res);
static void SendResponse(const HttpResponse& res, TCPConnection* conn);
static string GetContentType(const string& path);
static void SetErrCode(int err_code, HttpResponse* res);
static int SendFile(int file_fd, TCPConnection* conn);

static void LowerCase(string& str) {
  for (int i = 0; i < str.length(); i++) {
    if (str[i] >= 'A' && str[i] <= 'Z') {
      str[i] = static_cast<char>(str[i] + 32);
    }
  }
}

static void Trim(string& str) {
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

static int Split(string& str, const string& delim, vector<string>* out) {
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

static bool IsNumber(const string& str) {
  for (int i = 0; i < str.length(); i++) {
    if (str[i] < '0' || str[i] > '9') {
      return false;
    }
  }
  return true;
}

static void HandleSigint(int signum) {

}

static unordered_map<int, string> err_codes = {{404, "Not Found"},
                                               {405, "Method Not Allowed"},
                                               {505, "HTTP Version not supported"}};

HttpServer::HttpServer()
  : threadpool_(std::make_unique<ThreadPool>(100)), listen_port_(8000) { }

HttpServer::HttpServer(int max_thread, int listen_port)
  : threadpool_(std::make_unique<ThreadPool>(max_thread)), listen_port_(listen_port) { }

HttpServer::~HttpServer() { }

void HttpServer::Run() {
  PrepareToHandleSignal(SIGINT, HandleSigint);
  int listen_fd = CreateListenSocket(listen_port_, 100);
  if (listen_fd == -1) {
    std::cerr << "Failed to create listening socket. " << std::endl;
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
    unique_ptr<ThreadPool::Task> task = std::make_unique<HttpServerTask>(comm_fd);
    threadpool_->Dispatch(std::move(task));
  }
  threadpool_->KillThreads();
  std::cerr << "Server shut down" << std::endl;
}


HttpServerTask::HttpServerTask(int comm_fd) 
  : comm_fd_(comm_fd) { }
HttpServerTask::~HttpServerTask() { }
void HttpServerTask::Run() {
  ThreadLoop(comm_fd_);
}

static void ThreadLoop(int comm_fd) {
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

static bool ParseRequest(string& header, HttpRequest* req) {
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

static void ProcessRequest(const HttpRequest& req, HttpResponse* res) {
  if (req.method_ != "get") {
    SetErrCode(405, res);
    return;
  }
  if (req.protocol_ != "http/1.1") {
    SetErrCode(505, res);
    return;
  }
  string path;
  if (req.uri_ == "/") {
     path = DIR + "/index.html";
  } else {
    path = DIR + "/" + req.uri_;
  }
  int file = open(path.c_str(), O_RDONLY);
  if (file == -1) {
    std::cout << "failed to open "<< path << std::endl;
    SetErrCode(404, res);
    return;
  }
  res->protocol_ = "HTTP/1.1";
  res->status_code_ = 200;
  res->reason_phrase_ = "OK";
  res->AddHeader("Content-Type", GetContentType(path));
  res->has_file_ = true;
  off_t size = lseek(file, 0, SEEK_END);
  close(file);
  res->file_ = path;
  res->AddHeader("Content-Length", std::to_string(size));
}

static void SendResponse(const HttpResponse& res, TCPConnection* conn) {
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

static string GetContentType(const string& path) {
  string ext = ""; 
  for (int i = path.length() - 1; i >= 0; i--) {
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

static void SetErrCode(int err_code, HttpResponse* res) {
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

static int SendFile(int file_fd, TCPConnection* conn) {
  if (file_fd == -1) {
    return -1;
  }
  char* buf = new char[DATA_BATCH];
  int total_bytes = 0;
  ssize_t bytes_read = read(file_fd, buf, DATA_BATCH);
  while (bytes_read > 0) {
    conn->Send(string(buf, bytes_read));
    total_bytes += bytes_read;
    bytes_read = read(file_fd, buf, DATA_BATCH);
  }
  delete[] buf;
  return total_bytes;
}

} // end namespace WebServer

int main(int argc, char** argv) {
  int port = 80;
  int c;
  bool background = true;
  while ((c = getopt(argc, argv, "p:t")) != -1) {
    switch(c) {
      case 'p':
        if (!WebServer::IsNumber(string(optarg))) {
          std::cerr << "-p argument must be a number that specifies a port" << std::endl;
          return EXIT_FAILURE;
        }
        port = atoi(optarg);
        break;
      case 't':
        background = false;
        break;
      case '?':
        std::cerr << optopt << " is not an accepted argument." << std::endl;
        return -1;
      default:
        abort();
    }
  }
  if (optind >= argc) {
    std::cerr << "./httpserver run <directory>" << std::endl;
    std::cerr << "./httpserver end" << std::endl;
    return EXIT_FAILURE;
  }
  if (string(argv[optind]) == "end") {
    int fd = open("runlog/process.txt", O_RDWR);
    if (fd == -1) {
      return EXIT_FAILURE;
    }
    pid_t pid;
    read(fd, &pid, sizeof(pid_t));
    kill(pid, SIGINT);
    close(fd);
    remove("runlog/process.txt");
    return EXIT_SUCCESS;
  }
  if (string(argv[optind]) != "run" || optind + 1 >= argc) {
     std::cerr << "./httpserver run <directory>" << std::endl;
    return EXIT_FAILURE;
  }
  DIR = string(argv[optind + 1]);
  if (background) {
    pid_t pid = fork();
    if (pid == 0) {
      unique_ptr<WebServer::Server> server = std::make_unique<WebServer::HttpServer>(32, port);
      server->Run();
      exit(EXIT_SUCCESS);
    }
    mkdir("runlog", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    int fd = open("runlog/process.txt", O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
    write(fd, &pid, sizeof(pid_t));
    close(fd);
    std::cout << "httpserver is running" << std::endl;
    std::cout << "Listenting to port " << port << std::endl;
    std::cout << "Reading from directory " << DIR << std::endl;
    std::cout << "pid = " << pid << std::endl;
    return EXIT_SUCCESS;
  } 
  unique_ptr<WebServer::Server> server = std::make_unique<WebServer::HttpServer>(32, port);
  server->Run();
  return EXIT_SUCCESS;
}