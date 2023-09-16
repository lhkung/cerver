#ifndef TABULA_H_
#define TABULA_H_
#define INVALID_REQUEST 2

#include <string>
#include <unordered_map>
#include <memory>
#include "memtable.h"
#include "commitlog.h"

namespace KVStore {

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
    
    struct SSFile {
      std::string tableName;
      std::string uuid;
      std::string ext;
    };

    static void ParseSSFileName(const char* fileName, SSFile* ssFile);

  private:
    std::string dir_;
    std::unordered_map<std::string, std::unique_ptr<MemTable> > memtables_;
    std::unordered_map<std::string, std::unique_ptr<CommitLog> > commitlogs_;
    std::unordered_map<std::string, std::map<std::string, std::unique_ptr<SSIndex> > > ssIndices_;
    void Flush(MemTable* memtable);
    bool isValidTableName(const std::string& tableName);
    std::string MakeUniqueFileName(const std::string& tableName); 
    void PutSSIndexToMap(
      const std::string& tablerName,
      const std::string& uniqueFileName,
      std::unique_ptr<SSIndex> ssIndex
    );

};

} // KVStore

#endif