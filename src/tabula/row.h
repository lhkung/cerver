#ifndef ROW_H_
#define ROW_H_

#define ROW_METADATA_BYTES 16
#define COL_METADATA_BYTES 12
#include <string>
#include <unordered_map>
#include "tabulaenums.h"

namespace KVStore {

class Row {
public:
  struct RowMetaData {
    uint32_t rowNameLen;
    uint32_t numCols;
    uint64_t lastUpdated;
  };
  Row(const std::string& name);
  Row(const std::string& name, time_t lastUpdated);
  ~Row();
  int Put(
    const std::string& col, 
    const std::string& val
  );
  int PutWithoutUpdateTime(
    const std::string& col, 
    const std::string& val
  );
  int Get(
    const std::string& col, 
    std::string* val
  );
  int Delete(const std::string& col);
  bool operator == (const Row& right) const;
  bool operator != (const Row& right) const;
  
  // 4 bytes: row name length
  // 4 bytes: number of columns
  // 8 bytes: last updated time
  // n bytes: row name
  // Repeated:
  // 4 bytes: col name length
  // 8 bytes: col value length
  // n bytes: col name
  // n bytes: col value
  std::string Serialize();
  const std::string& Name();
  time_t LastUpdateTime();
  static std::unique_ptr<Row> Deserialize(int fd, uint64_t offset);
private:
  std::string name_;
  time_t lastUpdated_;
  std::unordered_map<std::string, std::string> columns_;
};

} // namespace KVStore


#endif
