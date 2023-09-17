#ifndef MEMTABLE_H_
#define MEMTABLE_H_

#define MEMTABLE_DEFAULT_CAPACITY 1024 * 1024 * 10 // 10 MB

#include <map>
#include <pthread.h>
#include <string>
#include <memory>
#include "row.h"
#include "ssindex.h"
#include "tabulaenums.h"

namespace KVStore {

class MemTable {
public:
  MemTable(const std::string& name);
  MemTable(const std::string& name, const uint64_t capacity);
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
  const std::string& Name();
  void Flush(
    const std::string& dir,
    const std::string& uniqueFileName,
    uint32_t indexFreq,
    SSIndex* ssIndex
  );

private:
  std::string name_;
  pthread_mutex_t lock_;
  std::map<std::string, std::unique_ptr<Row> > rows_;
  uint64_t size_;
  uint64_t capacity_;
};

} // namespace KVStore

#endif