#ifndef SS_INDEX_H_
#define SS_INDEX_H_
#define SS_INDEX_FILE_EXT ".ssx"
#define START_AND_END_ROW_METADATA_LENGTH 8
#define ROW_INDEX_METADATA_LENGTH 12

#include <string>
#include <map>
#include "tabulaenums.h"

namespace KVStore {

class SSIndex {

public:
  SSIndex( );
  SSIndex(const std::string& dir, const std::string& uniqueFileName);
  ~SSIndex();
  void AddIndex(const std::string& row, uint64_t offset);
  int GetOffset(const std::string& row, uint64_t* offset);
  void MemoizeStartAndEndRowFromMap();
  void MemoizeStartAndEndRowFromFile(int fd);
  void MemoizeStartAndEndRowFromFile();
  // 4 bytes start row name len
  // 4 bytes end row name len
  // n bytes start row
  // n bytes end row
  // Repeated:
  // 4 bytes: row name len
  // 8 bytes: offset
  // n bytes: row name
  int LoadIndexFileToMemory();
  void Flush();
  void Clear();
private:
  std::string indexFilePath_;
  bool loadedToMemory_;
  std::map<std::string, uint64_t> index_;
  std::string startRow_;
  std::string endRow_;
};
    
} // namespace KVStore


#endif