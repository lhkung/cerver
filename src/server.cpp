#include <string.h>
#include <arpa/inet.h>

#include "server.h"

using std::string;

namespace Cerver {

Server::Server() {};
Server::~Server() {};
int Server::CreateListenSocket(int port, int queue_capacity) {
  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1) {
    return listen_fd;
  }
  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &opt, sizeof(opt));
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(sockaddr_in));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(port);
  if (bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
    return -1;
  }
  if (listen(listen_fd, queue_capacity) == -1) {
    return -1;
  }
  return listen_fd;
}

int Server::AcceptConnection(int listen_fd, std::string* addr, int* port) {
  struct sockaddr_in clientaddr;
  socklen_t clientaddrlen = sizeof(clientaddr);
  int comm_fd = accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
  *addr = string(inet_ntoa(clientaddr.sin_addr));
  *port = ntohs(clientaddr.sin_port);
  return comm_fd;
}

void Server::PrepareToHandleSignal(int signal, void (*SignalHandler)(int)) {
  struct sigaction sa;
  sa.sa_handler = SignalHandler;
  sa.sa_flags = 0;
  sigemptyset (&sa.sa_mask);
  sigaction(signal, &sa, nullptr);
}

} // namespace Cerver