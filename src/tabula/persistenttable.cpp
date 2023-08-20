#include "persistenttable.h"

namespace Cerver {

PersistentTable::PersistentTable() { }
PersistentTable::~PersistentTable() { }
Table::Result PersistentTable::Put(const std::string& row, const std::string& col, const std::string& val) {

}
Table::Result PersistentTable::Get(const std::string& row, const std::string& col, std::string* val) {

}
Table::Result PersistentTable::Delete(const std::string& row, const std::string& col) {

}

} // namespace Cerver