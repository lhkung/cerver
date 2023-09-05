#ifndef ROW_H_
#define ROW_H_

#define SUCCESS 0
#define OVERWRITE 1
#define NOT_FOUND -1

#include <string>
#include <unordered_map>

namespace Cerver {
  
class Row {
public:
  Row(const std::string& name);
  ~Row();
  int Put(
    const std::string& col, 
    const std::string& val
  );
  int Get(
    const std::string& col, 
    std::string* val
  );
  int Delete(const std::string& col);
  
  // 4 bytes: row name length
  // n bytes: row name
  // 4 bytes: number of columns
  // Repeated:
  // 4 bytes: col name length
  // 4 bytes: col value length
  // n bytes: col name
  // n bytes: col value
  std::string Serialize();
  const std::string& Name();
private:
  std::string name_;
  std::unordered_map<std::string, std::string> columns_;
};

} // namespace Cerver


#endif
