#include "httprequest.h"

namespace Cerver {

HttpRequest::HttpRequest() { }
HttpRequest::~HttpRequest() { }
void HttpRequest::SetMethod(const std::string& method) {method_ = method;}
void HttpRequest::SetURI(const std::string& uri) {uri_ = uri;}
void HttpRequest::SetProtocol(const std::string& protocol) {protocol_ = protocol;}
void HttpRequest::SetBody(const std::string& body) {body_ = body;}
void HttpRequest::PutHeader(const std::string& k, const std::string& v) {headers_.insert({k, v});}
void HttpRequest::PutPathParam(const std::string& k, const std::string& v) {path_params_.insert({k, v});}
void HttpRequest::PutQueryParam(const std::string& k, const std::string& v) {query_params_.insert({k, v});}
const std::string& HttpRequest::Method() const {return method_;}
const std::string& HttpRequest::URI() const {return uri_;}
const std::string& HttpRequest::Protocol() const {return protocol_;}
const std::string& HttpRequest::Body() const {return body_;}
void HttpRequest::ClearPathParam() {path_params_.clear();}

int HttpRequest::Header(const std::string& k, std::string* v) const {
  if (headers_.find(k) == headers_.end()) {
    return -1;
  }
  *v = headers_.at(k);
  return 0;
}
int HttpRequest::PathParam(const std::string& k, std::string* v) const {
  if (path_params_.find(k) == path_params_.end()) {
    return -1;
  }
  *v = path_params_.at(k);
  return 0;
}
int HttpRequest::QueryParam(const std::string& k, std::string* v) const {
    if (query_params_.find(k) == query_params_.end()) {
    return -1;
  }
  *v = query_params_.at(k);
  return 0;
}
int HttpRequest::ContentLength() const {
  auto it = headers_.find("content-length");
  if (it == headers_.end()) {
    return -1;
  }
  return stoi(it->second);
}

} // namespace Cerver