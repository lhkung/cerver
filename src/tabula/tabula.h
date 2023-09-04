#ifndef TABULA_H_
#define TABULA_H_

#include <string>
#include <unordered_map>
#include <memory>
#include "memtable.h"
#include "commitlog.h"

namespace Cerver {

class Tabula {
  public:
    Tabula(const std::string& dir);
    ~Tabula();
    int Put(
      const std::string& tab, 
      const std::string& row, 
      const std::string& col, 
      const std::string& val
    );
    int Get(
      const std::string& tab, 
      const std::string& row, 
      const std::string& col, 
      std::string* val
    );
    int Delete(
      const std::string& tab, 
      const std::string& row, 
      const std::string& col
    );
    void Recover(const std::string& dir);

  private:
    std::string dir_;
    std::unordered_map<std::string, std::unique_ptr<MemTable> > memtables_;
    std::unordered_map<std::string, std::unique_ptr<CommitLog> > commitlogs_;
};

} // Cerver

#endif