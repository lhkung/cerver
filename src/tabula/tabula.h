#ifndef TABULA_H_
#define TABULA_H_

#include <string>
#include <unordered_map>
#include <memory>
#include "table.h"
#include "commitlog.h"

namespace Cerver {

class Tabula {
  public:
    Tabula(const std::string& dir);
    ~Tabula();
    Table::Result Put(
      const std::string& tab, 
      const std::string& row, 
      const std::string& col, 
      const std::string& val
    );
    Table::Result Get(
      const std::string& tab, 
      const std::string& row, 
      const std::string& col, 
      std::string* val
    );
    Table::Result Delete(
      const std::string& tab, 
      const std::string& row, 
      const std::string& col
    );
    void Recover(const std::string& dir);

  private:
    std::string dir_;
    std::unordered_map<std::string, std::unique_ptr<Table> > tables_;
    std::unordered_map<std::string, std::unique_ptr<CommitLog> > commitlogs_;
};

} // Cerver

#endif