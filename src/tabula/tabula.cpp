#include <dirent.h>
#include <iostream>
#include <string>
#include "tabula.h"
#include "../utils.h"

using std::string;

namespace KVStore {

Tabula::Tabula(const string& dir) : dir_(dir) {

}

Tabula::~Tabula() {

}

int Tabula::Put(
  const std::string& tab, 
  const std::string& row, 
  const std::string& col, 
  const std::string& val
) {
  auto memtabIt = memtables_.find(tab);
  auto commitIt = commitlogs_.find(tab);
  if (memtabIt == memtables_.end()) {
    memtabIt = memtables_.emplace(tab, std::make_unique<MemTable>(tab)).first;
    commitIt = commitlogs_.emplace(tab, std::make_unique<CommitLog>(tab, dir_ + "/tabula-data")).first;
  } else {
    Flush(memtabIt->second.get());
  }
  commitIt->second->LogPut(row, col, val);
  return memtabIt->second->Put(row, col, val);
}

int Tabula::Get(
  const std::string& tab, 
  const std::string& row, 
  const std::string& col, 
  std::string* val
) {
  auto memtabIt = memtables_.find(tab);
  if (memtabIt == memtables_.end()) {
    return NOT_FOUND;
  }
  return memtabIt->second->Get(row, col, val);
}

int Tabula::Delete(
  const std::string& tab, 
  const std::string& row, 
  const std::string& col
) {
  auto memtabIt = memtables_.find(tab);
  auto commitIt = commitlogs_.find(tab);
  if (memtabIt == memtables_.end()) {
    return NOT_FOUND;
  }
  commitIt->second->LogDelete(row, col);
  return memtabIt->second->Delete(row, col);
}

void Tabula::Recover(const std::string& dir) {
  DIR* dirPtr = opendir(dir.c_str());
  if (dirPtr == nullptr) {
    std::cout << "Failed to open directory: " << dir <<  std::endl;
    return;
  }
  struct dirent* entry = readdir(dirPtr);
  while (entry != nullptr) {
    if (Utils::EndsWith(entry->d_name, COMMMIT_LOG_FILE_EXT)) {
      string tablename = Utils::RemoveExt(string(entry->d_name));
      auto tableIt = memtables_.emplace(tablename, std::make_unique<MemTable>(tablename)).first;
      auto logIt = commitlogs_.emplace(tablename, std::make_unique<CommitLog>(dir.c_str(), entry->d_name)).first;
      logIt->second->Replay(tableIt->second.get());
    }
    entry = readdir(dirPtr);
  }
  closedir(dirPtr);
}

void Tabula::Flush(MemTable* memtable) {
  memtable->Flush(dir_ + "/tabula-data", 16);
  auto commitLogIt = commitlogs_.find(memtable->Name());
  if (commitLogIt == commitlogs_.end()) {
    return;
  }
  commitLogIt->second->Clear();
}

} // namespace KVStore