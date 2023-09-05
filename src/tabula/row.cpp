#include "row.h"

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

} // namespace Cerver