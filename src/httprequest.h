#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include <string>
#include <unordered_map>

namespace Cerver {

class HttpRequest {
public:
  HttpRequest();
  virtual ~HttpRequest();
  void SetMethod(const std::string& method);
  void SetURI(const std::string& uri);
  void SetProtocol(const std::string& body);
  void SetBody(const std::string& body);
  void PutHeader(const std::string& k, const std::string& v);
  void PutPathParam(const std::string& k, const std::string& v);
  void PutQueryParam(const std::string& k, const std::string& v);
  int Header(const std::string& k, std::string* v) const;
  int PathParam(const std::string& k, std::string* v) const;
  int QueryParam(const std::string& k, std::string* v) const;
  int ContentLength() const;
  void ClearPathParam();
  const std::string& Method() const;
  const std::string& URI() const;
  const std::string& Protocol() const;
  const std::string& Body() const;

  std::string method_;
  std::string uri_;
  std::string protocol_;
  std::string body_;
  std::unordered_map<std::string, std::string> headers_;
  std::unordered_map<std::string, std::string> path_params_;
  std::unordered_map<std::string, std::string> query_params_;
};

} // namespace Cerver

#endif