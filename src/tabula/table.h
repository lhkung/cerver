#ifndef TABLE_H_
#define TABLE_H_

#include <string>

namespace Cerver {

// Interface for Table.
class Table {
public:
  enum Result {
    SUCCESS,
    OVERWRITE,
    NOT_FOUND
  };
	Table() {}
	virtual ~Table() {}
	virtual Result Put(const std::string& row, const std::string& col, const std::string& val) = 0;
	virtual Result Get(const std::string& row, const std::string& col, std::string* val) = 0;
	virtual Result Delete(const std::string& row, const std::string& col) = 0;
};

} // namespace Cerver

#endif // TABLE_H_
