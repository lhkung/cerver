#ifndef PERSISTENT_TABLE_H_
#define PERSISTENT_TABLE_H_

#include <unordered_map>
#include "table.h"

namespace Cerver {

class PersistentTable : public Table {
  public:
    PersistentTable();
    ~PersistentTable();
    Result Put(const std::string& row, const std::string& col, const std::string& val) override;
    Result Get(const std::string& row, const std::string& col, std::string* val) override;
    Result Delete(const std::string& row, const std::string& col) override;
  
  private:
    std::unordered_map<std::string, off_t> rows_;
};

} // namespace Cerver

#endif