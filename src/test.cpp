#include "httpserver.h"
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

typedef std::function<string(string, string)> Route;
std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<Route> > > routes_;

void Put(const string& route, Route lambda) {
  auto it = routes_.find("put");
  if (it == routes_.end()) {
    routes_.emplace("put", unordered_map<string, unique_ptr<Route>>());
  } 
  routes_.at("put").emplace(route, std::make_unique<Route>(lambda));
}
void Get(const string& route, Route lambda) {
  auto it = routes_.find("get");
  if (it == routes_.end()) {
    routes_.emplace("get", unordered_map<string, unique_ptr<Route>>());
  } 
  routes_.at("get").emplace(route, std::make_unique<Route>(lambda));
}

int Split(const string& str, const string& delim, vector<string>* out) {
  size_t off = 0;
  while (true) {
    size_t index = str.find(delim, off);
    if (index == string::npos) {
      break;
    }
    if (index == off) {
      off++;
      continue;
    }
    out->push_back(str.substr(off, index - off));
    off = index + delim.length();
  }
  if (off < str.length()) {
    out->push_back(str.substr(off));
  }
  return out->size();
}

Route* CollectPathParam(string uri)  {
  vector<string> uri_split;
  Split(uri, "/", &uri_split);
  std::cout << "split uri length=" << uri_split.size() << std::endl;
  for (string s : uri_split) {
     std::cout << "split uri=" << s << std::endl;
  }
  const unordered_map<string, std::unique_ptr<Route> >& paths = routes_.at("get");
  for (auto it = paths.begin(); it != paths.end(); it++) {
    bool match = true;
    vector<string> path_split;
    Split(it->first, "/", &path_split);
    if (path_split.size() != uri_split.size()) {
      continue;
    }
    for (int i = 0; i < path_split.size(); i++) {
      if (path_split[i] == uri_split[i]) {
        continue;
      }
      if (path_split[i][0] == ':') {
        continue;
      }
      match = false;
      break;
    }
    if (match) {
      return it->second.get();
    }
  }
  return nullptr;
}

int main() {

    Get("/", [](string x, string y){return x + y;});
    Route* ret = CollectPathParam("/favicon.ico");
    if (ret == nullptr) {
        std::cout << "Not found" << std::endl;
    } else {
        std::cout << "Found" << std::endl;
        std::cout << (*ret)("hello", "world") << std::endl;
    }


    return 0;
}