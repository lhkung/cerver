#ifndef SERVER_H_
#define SERVER_H_

#include <string>
#include <signal.h>

namespace Cerver {

class Server {
  public:
    Server();
    virtual ~Server();
    virtual void Run() = 0;
    int CreateListenSocket(int port, int queue_capacity);
    int AcceptConnection(int listen_fd, std::string* addr, int* port);
    void PrepareToHandleSignal(int signal, void (*SignalHandler)(int));
};

} // namspace Cerver

#endif