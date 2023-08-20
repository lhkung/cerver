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
  auto tab_it = tables_.find(tab);
  auto commit_it = commitlogs_.find(tab);
  if (tab_it == tables_.end()) {
    tab_it = tables_.emplace(tab, std::make_unique<MemTable>()).first;
    commit_it = commitlogs_.emplace(tab, std::make_unique<CommitLog>(tab, dir_ + "/commitlogs")).first;
  }
  commit_it->second->LogPut(row, col, val);
  return tab_it->second->Put(row, col, val);
}

Table::Result Tabula::Get(const std::string& tab, const std::string& row, const std::string& col, std::string* val) {
  auto tab_it = tables_.find(tab);
  if (tab_it == tables_.end()) {
    return Table::NOT_FOUND;
  }
  return tab_it->second->Get(row, col, val);
}

Table::Result Tabula::Delete(const std::string& tab, const std::string& row, const std::string& col) {
  auto tab_it = tables_.find(tab);
  auto commit_it = commitlogs_.find(tab);
  if (tab_it == tables_.end()) {
    return Table::NOT_FOUND;
  }
  commit_it->second->LogDelete(row, col);
  return tab_it->second->Delete(row, col);
}

void Tabula::Recover(const std::string& dir) {
  DIR* dir_ptr = opendir(dir.c_str());
  if (dir_ptr == nullptr) {
    std::cout << "Failed to open directory: " << dir <<  std::endl;
    return;
  }
  struct dirent* entry = readdir(dir_ptr);
  while (entry != nullptr) {
    if (EndsWith(entry->d_name, ".commitlog")) {
      string tablename = RemoveExt(string(entry->d_name));
      auto table_it = tables_.emplace(tablename, std::make_unique<MemTable>()).first;
      auto log_it = commitlogs_.emplace(tablename, std::make_unique<CommitLog>(dir.c_str(), entry->d_name)).first;
      log_it->second->Replay(table_it->second.get());
    }
    entry = readdir(dir_ptr);
  }
  closedir(dir_ptr);
}

} // namespace Cerver