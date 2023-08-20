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
  auto row_it = memtable_.find(row);
  if (row_it == memtable_.end()) {
    row_it = memtable_.emplace(string(row), unordered_map<string, string>()).first;
  }
  auto col_it = row_it->second.find(col);
  if (col_it == row_it->second.end()) {
    row_it->second.emplace(string(col), string(val));
    pthread_mutex_unlock(&lock_);
    return SUCCESS;
  }
  col_it->second = val;
  pthread_mutex_unlock(&lock_);
  return OVERWRITE;
}
Table::Result MemTable::Get(const std::string& row, const std::string& col, std::string* val) {
  pthread_mutex_lock(&lock_);
  auto row_it = memtable_.find(row);
  if (row_it == memtable_.end()) {
    pthread_mutex_unlock(&lock_);
    return NOT_FOUND;
  }
  auto col_it = row_it->second.find(col);
  if (col_it == row_it->second.end()) {
    pthread_mutex_unlock(&lock_);
    return NOT_FOUND;
  }
  *val = col_it->second;
  pthread_mutex_unlock(&lock_);
  return SUCCESS;
}
Table::Result MemTable::Delete(const std::string& row, const std::string& col) {
  pthread_mutex_lock(&lock_);
  auto row_it = memtable_.find(row);
  if (row_it == memtable_.end()) {
    pthread_mutex_unlock(&lock_);
    return NOT_FOUND;
  }
  auto col_it = row_it->second.find(col);
  if (col_it == row_it->second.end()) {
    pthread_mutex_unlock(&lock_);
    return NOT_FOUND;
  }
  row_it->second.erase(col_it);
  pthread_mutex_unlock(&lock_);
  return SUCCESS;
}

} // namespace Cerver