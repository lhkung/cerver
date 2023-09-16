#ifndef SS_INDEX_H_
#define SS_INDEX_H_
#define SS_INDEX_FILE_EXT ".ssx"

#include <string>
#include <unordered_map>

namespace KVStore {

class SSIndex {

public:
  SSIndex( );
  SSIndex(const std::string& dir, const std::string& uniqueFileName);
  ~SSIndex();
  void AddIndex(const std::string& row, uint64_t offset);
  // 4 bytes: row name len
  // 8 bytes: offset
  // n bytes: row name
  int LoadIndexFileToMemory();
  void Flush();
  void Clear();
private:
  std::string indexFilePath_;
  bool loadedToMemory_;
  std::unordered_map<std::string, uint64_t> index_;
};
    
} // namespace KVStore


#endif