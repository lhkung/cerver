#include "memtable.h"

using std::string;
using std::unordered_map;

namespace Cerver {

MemTable::MemTable() {
  pthread_mutex_init(&lock_, nullptr);
}
MemTable::~MemTable() {
  pthread_mutex_destroy(&lock_);
}
Table::Result MemTable::Put(const std::string& row, const std::string& col, const std::string& val) {
  pthread_mutex_lock(&lock_);
  auto rowIt = memtable_.find(row);
  if (rowIt == memtable_.end()) {
    rowIt = memtable_.emplace(string(row), unordered_map<string, string>()).first;
  }
  auto colIt = rowIt->second.find(col);
  if (colIt == rowIt->second.end()) {
    rowIt->second.emplace(string(col), string(val));
    pthread_mutex_unlock(&lock_);
    return SUCCESS;
  }
  colIt->second = val;
  pthread_mutex_unlock(&lock_);
  return OVERWRITE;
}
Table::Result MemTable::Get(const std::string& row, const std::string& col, std::string* val) {
  pthread_mutex_lock(&lock_);
  auto rowIt = memtable_.find(row);
  if (rowIt == memtable_.end()) {
    pthread_mutex_unlock(&lock_);
    return NOT_FOUND;
  }
  auto colIt = rowIt->second.find(col);
  if (colIt == rowIt->second.end()) {
    pthread_mutex_unlock(&lock_);
    return NOT_FOUND;
  }
  *val = colIt->second;
  pthread_mutex_unlock(&lock_);
  return SUCCESS;
}
Table::Result MemTable::Delete(const std::string& row, const std::string& col) {
  pthread_mutex_lock(&lock_);
  auto rowIt = memtable_.find(row);
  if (rowIt == memtable_.end()) {
    pthread_mutex_unlock(&lock_);
    return NOT_FOUND;
  }
  auto colIt = rowIt->second.find(col);
  if (colIt == rowIt->second.end()) {
    pthread_mutex_unlock(&lock_);
    return NOT_FOUND;
  }
  rowIt->second.erase(colIt);
  pthread_mutex_unlock(&lock_);
  return SUCCESS;
}

} // namespace Cerver