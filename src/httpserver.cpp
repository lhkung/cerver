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

namespace Cerver {

volatile bool running = true;
volatile bool stat = false;

static void LowerCase(string& str) {
  for (uint i = 0; i < str.length(); i++) {
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

 static const char* GetTime() {
  time_t now = time(0);
  char* time_str = ctime(&now);
  int len = strlen(time_str);
  time_str[len - 1] = '\0';
  return strcat(time_str, ": ");
}

static bool IsNumber(const string& str) {
  for (uint i = 0; i < str.length(); i++) {
    if (str[i] < '0' || str[i] > '9') {
      return false;
    }
  }
  return true;
}

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
    cache_(std::make_unique<LRUCache>(cache_size)),
    dir_(dir),
    listen_port_(listen_port),
    stat_() {
    pthread_mutex_init(&loglock_, nullptr);
}

HttpServer::~HttpServer() {
   pthread_mutex_destroy(&loglock_);
}

void HttpServer::Run() {
  PrepareToHandleSignal(SIGINT, HandleSignal);
  PrepareToHandleSignal(SIGUSR1, HandleSignal);
  pthread_mutex_lock(&loglock_);
  *log_ << GetTime() << "Server starts\n";
  pthread_mutex_unlock(&loglock_);
  int listen_fd = CreateListenSocket(listen_port_, 100);
  if (listen_fd == -1) {
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
    pthread_mutex_lock(&loglock_);
    stat_.num_conn_++;
    *log_ << GetTime() << "Connection from " << addr << ":" << port << "\n";
    pthread_mutex_unlock(&loglock_);
    unique_ptr<ThreadPool::Task> task = std::make_unique<HttpServerTask>(comm_fd, this);
    threadpool_->Dispatch(std::move(task));
  }
  threadpool_->KillThreads();
  pthread_mutex_lock(&loglock_);
  *log_ << GetTime() << "Server shut down\n";
  pthread_mutex_unlock(&loglock_);
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
    bool req_valid = ParseRequest(header, &req);
    pthread_mutex_lock(&loglock_);
    *log_ << GetTime() << req.method_ << " " << req.uri_ << " " << req.protocol_ << "\n";
    pthread_mutex_unlock(&loglock_);
    if (!req_valid) {
      SetErrCode(404, &res);
      SendResponse(res, &conn);
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
    ProcessRequest(req, &res);
    SendResponse(res, &conn);
  }
  conn.Close();
  pthread_mutex_lock(&loglock_);
  stat_.num_conn_--;
  *log_ << GetTime() << "Connection closed\n";
  pthread_mutex_unlock(&loglock_);
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
  for (uint i = 1; i < lines.size(); i++) {
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
  res->protocol_ = "HTTP/1.1";
  res->status_code_ = 200;
  res->reason_phrase_ = "OK";
  res->AddHeader("Content-Type", GetContentType(path));
  res->has_file_ = true;
  off_t size = lseek(file, 0, SEEK_END);
  close(file);
  res->file_size_ = static_cast<size_t>(size);
  res->file_ = path;
  res->uri_ = req.uri_;
  res->AddHeader("Content-Length", std::to_string(size));
}

void HttpServer::SendResponse(const HttpResponse& res, TCPConnection* conn) {
  *log_ << GetTime() << "Sending response header " << res.status_code_ << "\n";
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
    SendFile(res, conn);
  }
  pthread_mutex_lock(&loglock_);
  *log_ << GetTime() << "Response sent\n";
  pthread_mutex_unlock(&loglock_);
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

int HttpServer::SendFile(const HttpResponse& res, TCPConnection* conn) {
  string val;
  if (cache_->Get(res.uri_, &val) == 0) {
    if (val.length() == res.file_size_) {
      conn->Send(val);
      pthread_mutex_lock(&loglock_);
      stat_.num_cache_++;
      stat_.num_read_++;
      stat_.byte_cache_ += val.length();
      stat_.byte_read_ += val.length();
      pthread_mutex_unlock(&loglock_);
      return val.length();
    }
    pthread_mutex_lock(&loglock_);
    stat_.incomplete_cache_entry_++;
    pthread_mutex_unlock(&loglock_);
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
  pthread_mutex_lock(&loglock_);
  stat_.num_read_++;
  stat_.byte_read_ += total_bytes;
  pthread_mutex_unlock(&loglock_);
  return total_bytes;
}

void HttpServer::PrintStat() {
  std::cout << "HttpServer status\n";
  std::cout << "Listening on port " << listen_port_ << "\n";
  std::cout << "Number of threads: " << threadpool_->num_threads_running_ << "\n";
  std::cout << "Number of active connections: " << stat_.num_conn_ << "\n";
  std::cout << "Number of tasks in work queue: " << threadpool_->work_queue_.size() << "\n";
  std::cout << "Cache size: " << cache_->Size() << "\n";
  std::cout << "Cache entries: " << cache_->Entry() << "\n";
  std::cout << "Cache hit rate (time): " << stat_.num_cache_ << "/" << stat_.num_read_ << " = " <<  static_cast<float>(stat_.num_cache_) / static_cast<float>(stat_.num_read_) << "\n";
  std::cout << "Cache hit rate (byte): " << stat_.byte_cache_ << "/" << stat_.byte_read_ << " = " <<  static_cast<float>(stat_.byte_cache_) / static_cast<float>(stat_.byte_read_) << "\n";
  std::cout << "Cache entry incomplete: " << stat_.incomplete_cache_entry_ << std::endl;
  stat = false;
} 

} // end namespace WebServer

int main(int argc, char** argv) {
  int port = 80;
  int c;
  bool background = true;
  while ((c = getopt(argc, argv, "p:t")) != -1) {
    switch(c) {
      case 'p':
        if (!Cerver::IsNumber(string(optarg))) {
          std::cout << "-p argument must be a number that specifies a port" << std::endl;
          return EXIT_FAILURE;
        }
        port = atoi(optarg);
        break;
      case 't':
        background = false;
        break;
      case '?':
        std::cout << optopt << " is not an accepted argument." << std::endl;
        return 1;
      default:
        abort();
    }
  }
  if (optind >= argc) {
    std::cout << "./httpserver run <directory>\n";
    std::cout << "./httpserver end" << std::endl;
    return EXIT_FAILURE;
  }
  if (string(argv[optind]) == "end") {
    int fd = open("runlog/process.txt", O_RDWR);
    if (fd == -1) {
      std::cout << "No HttpServer is running" << std::endl;
      return EXIT_FAILURE;
    }
    pid_t pid;
    read(fd, &pid, sizeof(pid_t));
    kill(pid, SIGINT);
    close(fd);
    remove("runlog/process.txt");
    return EXIT_SUCCESS;
  }
  if (string(argv[optind]) == "stat") {
     int fd = open("runlog/process.txt", O_RDWR);
    if (fd == -1) {
      std::cout << "No HttpServer is running" << std::endl;
      return EXIT_FAILURE;
    }
    pid_t pid;
    read(fd, &pid, sizeof(pid_t));
    kill(pid, SIGUSR1);
    close(fd);
    return EXIT_SUCCESS;
  }
  if (string(argv[optind]) != "run" || optind + 1 >= argc) {
     std::cout << "./httpserver run <directory>" << std::endl;
    return EXIT_FAILURE;
  }
  mkdir("runlog", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (background) {
    pid_t pid = fork();
    if (pid == 0) {
      unique_ptr<Cerver::Server> server = std::make_unique<Cerver::HttpServer>(64, port, 1048576, string(argv[optind + 1]), "runlog");
      server->Run();
      exit(EXIT_SUCCESS);
    }
    int fd = open("runlog/process.txt", O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
    write(fd, &pid, sizeof(pid_t));
    close(fd);
    std::cout << "httpserver is running\n";
    std::cout << "Listenting to port " << port << "\n";
    std::cout << "Reading from directory " << argv[optind + 1] << "\n";
    std::cout << "pid = " << pid << std::endl;
    return EXIT_SUCCESS;
  }
  unique_ptr<Cerver::Server> server = std::make_unique<Cerver::HttpServer>(64, port, 1048576, string(argv[optind + 1]), "runlog");
  server->Run();
  return EXIT_SUCCESS;
}