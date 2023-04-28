#include "httpresponse.h"

using std::string;

namespace Cerver {

HttpResponse::HttpResponse(TCPConnection* conn) : status_code_(200), reason_phrase_("OK"), has_body_(false), body_(""), conn_(conn) { }
HttpResponse::HttpResponse() : HttpResponse(nullptr){ }
HttpResponse::~HttpResponse() { }
void HttpResponse::PutHeader(const string& k, const string& v) {headers_.insert({k, v});}
void HttpResponse::SetProtocol(const string& protocol) {protocol_ = protocol;}
void HttpResponse::SetStatusCode(int status_code, const string& reason_phrase) {status_code_ = status_code;reason_phrase_ = reason_phrase;}
void HttpResponse::SetBody(const string& body) {has_body_ = true; body_ = body;}
void HttpResponse::AppendBody(const std::string& body) {has_body_ = true; body_ += body;}
void HttpResponse::SetContentType(const string& type) {PutHeader("Content-Type", type);}
int HttpResponse::StatusCode() const {return status_code_;}
const std::string& HttpResponse::Reason() const {return reason_phrase_;}
const string& HttpResponse::Body() const {return body_;}
bool HttpResponse::Written() const {return written_;}
bool HttpResponse::HasBody() const {return has_body_;}
const std::unordered_map<std::string, std::string>& HttpResponse::Headers() const {return headers_;}
void HttpResponse::write(const string& content) {
    if (!written_) {
    written_ = true;
    PutHeader("Connection", "close");
    conn_->Send("HTTP/1.1 " + std::to_string(status_code_) + " " + reason_phrase_ + "\r\n");
    for (auto it = headers_.begin(); it != headers_.end(); it++) {
        conn_->Send(it->first + ": " + it->second + "\r\n");
    }
    conn_->Send("\r\n");
    }
    conn_->Send(content);
}

} // namespace Cerver