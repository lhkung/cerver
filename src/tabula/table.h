#ifndef TABLE_H_
#define TABLE_H_
#define SUCCESS 0
#define OVERWRITE 1
#define NOT_FOUND -1

#include <string>

namespace Cerver {

class Table {
public:
	Table() {}
	virtual ~Table() {}
	virtual int Put(const std::string& row, const std::string& col, const std::string& val) = 0;
	virtual int Get(const std::string& row, const std::string& col, std::string* val) = 0;
	virtual int Delete(const std::string& row, const std::string& col) = 0;
};

} // namespace Cerver

#endif // TABLE_H_
