#ifndef TABULA_H_
#define TABULA_H_

#include <string>
#include <unordered_map>
#include <memory>
#include "memtable.h"
#include "commitlog.h"
#include "tabulaenums.h"

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
    // One memtable per table
    std::unordered_map<std::string, std::unique_ptr<MemTable> > memtables_;
    // One commit log per table
    std::unordered_map<std::string, std::unique_ptr<CommitLog> > commitlogs_;
    // Might have more than one SSIndex per table
    // table name -> unique file name -> SSIndex
    std::unordered_map<std::string, std::unique_ptr<std::map<std::string, std::unique_ptr<SSIndex> > > > ssIndices_;
    void Flush(MemTable* memtable);
    bool isValidTableName(const std::string& tableName);
    std::string MakeUniqueFileName(const std::string& tableName); 
    void PutSSIndexToMap(
      const std::string& tablerName,
      const std::string& uniqueFileName,
      std::unique_ptr<SSIndex> ssIndex
    );
    int GetFromDisk(
      const std::string& tab, 
      const std::string& row, 
      const std::string& col, 
      std::string* val
    );
    std::unique_ptr<Row> Tabula::ReadRowFromSSTable(
      const std::string& fileName,
      uint64_t offset
    );

};

} // KVStore

#endif