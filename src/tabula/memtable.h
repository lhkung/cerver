#ifndef MEMTABLE_H_
#define MEMTABLE_H_

#include <unordered_map>
#include <pthread.h>
#include "table.h"

namespace Cerver {

class MemTable : public Table {
public:
  MemTable();
  MemTable(uint64_t capacity);
  ~MemTable();
  int Put(
    const std::string& row, 
    const std::string& col, 
    const std::string& val
  ) override;
  int Get(
    const std::string& row,
    const std::string& col,
    std::string* val
  ) override;
  int Delete(
    const std::string& row, 
    const std::string& col
  ) override;
  uint64_t Size();
  uint64_t Capacity();
  void Flush();

private:
  pthread_mutex_t lock_;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string> > memtable_;
  uint64_t size_;
  uint64_t capacity_;
};

} // namespace Cerver

#endif