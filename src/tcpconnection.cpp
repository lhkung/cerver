#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>

#include "tcpconnection.h"

using std::string;

namespace Cerver {

TCPConnection::TCPConnection() : sockfd_(-1) {}
TCPConnection::TCPConnection(int sockfd) : sockfd_(sockfd) {}
TCPConnection::~TCPConnection() {}

int TCPConnection::Connect(const string& addr, int port) {
  int sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    return -1;
  }
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(sockaddr_in));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);
  inet_pton(AF_INET, addr.c_str(), &(servaddr.sin_addr));
  int ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if (ret == -1) {
    return -1;
  }
  sockfd_ = sockfd;
  return sockfd;
}

int TCPConnection::Send(const string& msg) {
  return WriteToSocket(sockfd_, msg);
}

int TCPConnection::ReadFromSocket() {
  int res;
  char buffer[1024];
  res = read(sockfd_, buffer, 1024);
  if (res == -1) {
    return -1;
  }
  if (res == 0) {
    return 0;
  }
  buff_ += string(buffer, res);
  return res;
}

int TCPConnection::ReadUntilDoubleCRLF(string* msg) {
  size_t index = buff_.find(DOUBLE_CRLF);
  if (index != string::npos) {
    *msg = buff_.substr(0, index);
    buff_ = buff_.substr(index + DOUBLE_CRLF.length());
    return msg->length();
  }
  bool end = false;
  while (!end) {
    int size_read = ReadFromSocket();
    if (size_read <= 0) {
      return size_read;
    }
    index = buff_.find(DOUBLE_CRLF);
    if (index != string::npos) {
      *msg = buff_.substr(0, index);
      buff_ = buff_.substr(index + DOUBLE_CRLF.length());
      end = true; 
    }
  }
  return msg->length();
}

void TCPConnection::ReadSize(size_t size, string* msg) {
  if (buff_.length() >= size) {
    *msg = buff_.substr(0, size);
    buff_ = buff_.substr(size);
    return;
  }
  while (buff_.length() < size) {
    char buffer[1024];
    int res = read(sockfd_, buffer, 1024);
    if (res == -1) {
      break;
    }
    buff_ += string(buffer, res);
  }
  *msg = buff_.substr(0, size);
  buff_ = buff_.substr(size);
}

void TCPConnection::Close() {
  close(sockfd_);
}

void TCPConnection::SetSocketFd(int sockfd) {
  this->sockfd_ = sockfd;
}

int TCPConnection::WriteToSocket(int fd, const string& content) {
  int res;
  size_t bytes_written = 0;
  while (bytes_written < content.length()) {
    res = write(fd, content.c_str() + bytes_written, content.length() - bytes_written);
    if (res == -1) {
      if (errno == EAGAIN) {continue;}
      break;
    }
    if (res == 0) {
        break;
    }
    bytes_written += res;
  }
  return bytes_written;
}

} // end namespace WebServer
