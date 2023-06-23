#ifndef TCP_CONNECTION_H_
#define TCP_CONNECTION_H_

#include <string>

namespace Cerver {

static const std::string DOUBLE_CRLF = "\r\n\r\n";

class TCPConnection {
  public:
    TCPConnection();
    TCPConnection(int sockfd);
    ~TCPConnection();
    // Establishes a TCP connection to the address and port.
    // Returns a file descriptor for the connection.
    int Connect(const std::string& addr, int port);
    // Sends [msg] through the connection
    int Send(const std::string& msg);
    // Reads until <CR><LF>. May block. Returns result through [msg]
    int ReadUntilDoubleCRLF(std::string* msg);
    // Reads [size] from socket. May block. Returns result through [msg]
    void ReadSize(size_t size, std::string* msg);
    // Ends connection and closes socket
    void Close();
    void SetSocketFd(int sockfd);

  private:
    int ReadFromSocket();
    int WriteToSocket(int fd, const std::string& content);
    int sockfd_;
    std::string buff_;
};

} // namespace Cerver

#endif