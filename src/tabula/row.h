#ifndef ROW_H_
#define ROW_H_

#define SUCCESS 0
#define OVERWRITE 1
#define NOT_FOUND -1
#define ROW_METADATA_BYTES 16
#define COL_METADATA_BYTES 12
#include <string>
#include <unordered_map>

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
  
  // row name length
  // number of columns
  // last updated time
  // row name
  // Repeated:
  // col name length
  // col value length
  // col name
  // col value
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
