#include <fcntl.h>
#include <unistd.h>
#include "memtable.h"

using std::string;
using std::unordered_map;

namespace Cerver {

MemTable::MemTable(const string& name) : MemTable(name, MEMTABLE_DEFAULT_CAPACITY) { }
MemTable::MemTable(
  const string& name, 
  const uint64_t capacity
) : name_(name),
    size_(0), 
    capacity_(capacity) {
  pthread_mutex_init(&lock_, nullptr);
}
MemTable::~MemTable() {
  pthread_mutex_destroy(&lock_);
}
int MemTable::Put(
  const std::string& row, 
  const std::string& col, 
  const std::string& val
) {
  pthread_mutex_lock(&lock_);
  auto rowIt = rows_.find(row);
  if (rowIt == rows_.end()) {
    rowIt = rows_.emplace(row, std::make_unique<Row>(row)).first;
  }
  int res = rowIt->second->Put(col, val);
  pthread_mutex_unlock(&lock_);
  return res;
}

int MemTable::Get(
  const std::string& row, 
  const std::string& col,
  std::string* val
) {
  pthread_mutex_lock(&lock_);
  auto rowIt = rows_.find(row);
  if (rowIt == rows_.end()) {
    pthread_mutex_unlock(&lock_);
    return NOT_FOUND;
  }
  int res = rowIt->second->Get(col, val);
  pthread_mutex_unlock(&lock_);
  return res;
}

int MemTable::Delete(
  const std::string& row,
  const std::string& col
) {
  pthread_mutex_lock(&lock_);
  auto rowIt = rows_.find(row);
  if (rowIt == rows_.end()) {
    pthread_mutex_unlock(&lock_);
    return NOT_FOUND;
  }
  int res = rowIt->second->Delete(col);
  pthread_mutex_unlock(&lock_);
  return res;
}

uint64_t MemTable::Size() {
  return size_;
}

uint64_t MemTable::Capacity() {
  return capacity_;
}

const string& MemTable::Name () {
  return name_;
}

void MemTable::Flush(const string& dir, uint32_t indexFreq) {
  string ssTablePath = dir + "/" + name_ + ".sst";
  string indexPath = dir + "/" + name_ + ".ssx";
  int ssTableFd = open(ssTablePath.c_str(), O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
  int indexFd = open(indexPath.c_str(), O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
  uint64_t offset = 0;
  uint32_t buildIndex = 0;
  for (auto it = rows_.begin(); it != rows_.end(); it++) {
    string serializedRow = it->second->Serialize();
    write(ssTableFd, serializedRow.c_str(), serializedRow.length());
    if (buildIndex++ % indexFreq == 0) {
      uint32_t rowNameLen = static_cast<uint32_t>(it->second->Name().length());
      write(indexFd, &rowNameLen, sizeof(rowNameLen));
      write(indexFd, it->second->Name().c_str(), rowNameLen);
      write(indexFd, &offset, rowNameLen);
    }
    offset += serializedRow.length();
  }
  close(ssTableFd);
}

} // namespace Cerver