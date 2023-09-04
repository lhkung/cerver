#ifndef MEMTABLE_H_
#define MEMTABLE_H_

#define SUCCESS 0
#define OVERWRITE 1
#define NOT_FOUND -1

#include <unordered_map>
#include <pthread.h>
#include <string>

namespace Cerver {

class MemTable {
public:
  MemTable();
  MemTable(uint64_t capacity);
  ~MemTable();
  int Put(
    const std::string& row, 
    const std::string& col, 
    const std::string& val
  );
  int Get(
    const std::string& row,
    const std::string& col,
    std::string* val
  );
  int Delete(
    const std::string& row, 
    const std::string& col
  );
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