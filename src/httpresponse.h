#ifndef HTTP_RESPONSE_H_
#define HTTP_RESPONSE_H_

#include <string>
#include <unordered_map>
#include "tcpconnection.h"

namespace Cerver {

class HttpResponse {
public:
  HttpResponse();
  HttpResponse(TCPConnection* conn);
  virtual ~HttpResponse();
  void PutHeader(const std::string& k, const std::string& v);
  void SetProtocol(const std::string& protocol);
  void SetStatusCode(int status_code, const std::string& reason_phrase);
  void SetBody(const std::string& body);
  void AppendBody(const std::string& body);
  void SetContentType(const std::string& type);
  void write(const std::string& content);
  bool Written() const;
  int StatusCode() const;
  const std::string& Reason() const;
  const std::string& Body() const;
  std::string* BodyPtr();
  void UseBody();
  bool HasBody() const;
  const std::unordered_map<std::string, std::string>& Headers() const; 
private:
  int status_code_;
  std::string reason_phrase_;
  std::string protocol_;
  std::unordered_map<std::string, std::string> headers_;
  bool has_body_;
  std::string body_;
  TCPConnection* conn_;
  bool written_;
};

} // namespace Cerver

#endif