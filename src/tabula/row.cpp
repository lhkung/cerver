#include "row.h"
#include <sstream>

using std::string;

namespace Cerver {

Row::Row(const std::string& name) : name_(name) {}
Row::~Row() {}

int Row::Put(
  const std::string& col, 
  const std::string& val
) {
  auto it = columns_.find(col);
  if (it == columns_.end()) {
    it = columns_.emplace(col, val).first;
    return SUCCESS;
  }
  it->second = val;
  return OVERWRITE;
}

int Row::Get(
  const std::string& col, 
  std::string* val
) {
  auto it = columns_.find(col);
  if (it == columns_.end()) {
    return NOT_FOUND;
  }
  *val = it->second;
  return SUCCESS;
}

int Row::Delete(const std::string& col) {
   auto it = columns_.find(col);
   if (it == columns_.end()) {
    return NOT_FOUND;
  }
  columns_.erase(it);
  return SUCCESS;
}

string Row::Serialize() {
  std::stringstream ss;
  uint32_t rowNameLen = static_cast<uint32_t>(name_.length());
  char* rowNameLenPtr = reinterpret_cast<char*>(&rowNameLen);
  ss.write(rowNameLenPtr, sizeof(uint32_t));
  ss.write(name_.c_str(), rowNameLen);
  for (auto it = columns_.begin(); it != columns_.end(); it++) {
    uint32_t colNameLen = static_cast<uint32_t>(it->first.length());
    uint32_t valLen = static_cast<uint32_t>(it->second.length());
    char* colNameLenPtr  = reinterpret_cast<char*>(&colNameLen);
    char* valLenPtr = reinterpret_cast<char*>(&valLen);
    ss.write(colNameLenPtr, sizeof(uint32_t));
    ss.write(valLenPtr, sizeof(uint32_t));
    ss.write(it->first.c_str(), colNameLen);
    ss.write(it->second.c_str(), valLen);
  }
  return ss.str();
}



} // namespace Cerver