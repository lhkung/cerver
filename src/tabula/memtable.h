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
    Result Put(const std::string& row, const std::string& col, const std::string& val) override;
    Result Get(const std::string& row, const std::string& col, std::string* val) override;
    Result Delete(const std::string& row, const std::string& col) override;
    uint64_t Size();
    uint64_t Capacity();
  
  private:
    pthread_mutex_t lock_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string> > memtable_;
    uint64_t size_;
    uint64_t capacity_;
};

} // namespace Cerver

#endif