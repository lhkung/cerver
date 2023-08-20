#include <dirent.h>
#include <iostream>
#include <string>
#include "tabula.h"
#include "../utils.h"

using std::string;

namespace Cerver {

Tabula::Tabula(const string& dir) : dir_(dir) {

}

Tabula::~Tabula() {

}
Table::Result Tabula::Put(const std::string& tab, const std::string& row, const std::string& col, const std::string& val) {
  auto tabIt = tables_.find(tab);
  auto commitIt = commitlogs_.find(tab);
  if (tabIt == tables_.end()) {
    tabIt = tables_.emplace(tab, std::make_unique<MemTable>()).first;
    commitIt = commitlogs_.emplace(tab, std::make_unique<CommitLog>(tab, dir_ + "/commitlogs")).first;
  }
  commitIt->second->LogPut(row, col, val);
  return tabIt->second->Put(row, col, val);
}

Table::Result Tabula::Get(const std::string& tab, const std::string& row, const std::string& col, std::string* val) {
  auto tabIt = tables_.find(tab);
  if (tabIt == tables_.end()) {
    return Table::NOT_FOUND;
  }
  return tabIt->second->Get(row, col, val);
}

Table::Result Tabula::Delete(const std::string& tab, const std::string& row, const std::string& col) {
  auto tabIt = tables_.find(tab);
  auto commitIt = commitlogs_.find(tab);
  if (tabIt == tables_.end()) {
    return Table::NOT_FOUND;
  }
  commitIt->second->LogDelete(row, col);
  return tabIt->second->Delete(row, col);
}

void Tabula::Recover(const std::string& dir) {
  DIR* dirPtr = opendir(dir.c_str());
  if (dirPtr == nullptr) {
    std::cout << "Failed to open directory: " << dir <<  std::endl;
    return;
  }
  struct dirent* entry = readdir(dirPtr);
  while (entry != nullptr) {
    if (EndsWith(entry->d_name, ".commitlog")) {
      string tablename = RemoveExt(string(entry->d_name));
      auto tableIt = tables_.emplace(tablename, std::make_unique<MemTable>()).first;
      auto logIt = commitlogs_.emplace(tablename, std::make_unique<CommitLog>(dir.c_str(), entry->d_name)).first;
      logIt->second->Replay(tableIt->second.get());
    }
    entry = readdir(dirPtr);
  }
  closedir(dirPtr);
}

} // namespace Cerver