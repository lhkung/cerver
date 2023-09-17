#include <dirent.h>
#include <iostream>
#include <string>
#include <fcntl.h>
#include "tabula.h"
#include "../utils.h"

using std::string;

namespace KVStore {

Tabula::Tabula(const string& dir) : dir_(dir) {}

Tabula::~Tabula() {}

int Tabula::Put(
  const std::string& tab, 
  const std::string& row, 
  const std::string& col, 
  const std::string& val
) {
  if (!isValidTableName(tab)) {
    return INVALID_REQUEST;
  }
  auto memtabIt = memtables_.find(tab);
  auto commitIt = commitlogs_.find(tab);
  if (memtabIt == memtables_.end()) {
    memtabIt = memtables_.emplace(tab, std::make_unique<MemTable>(tab)).first;
    commitIt = commitlogs_.emplace(tab, std::make_unique<CommitLog>(tab, dir_ + "/tabula-data")).first;
  } else {
    if (memtabIt->second->Size() + val.length() > memtabIt->second->Capacity()) {
      Flush(memtabIt->second.get());
    }
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
    // No such table
    return NOT_FOUND;
  }
  int ret = memtabIt->second->Get(row, col, val);
  if (ret == SUCCESS) {
    // Data is found in memtable.
    return ret;
  }
  return GetFromDisk(
    tab, 
    row, 
    col, 
    val
  );
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
    return;
  }
  struct dirent* entry = readdir(dirPtr);
  while (entry != nullptr) {
    SSFile ssFile;
    ParseSSFileName(entry->d_name, &ssFile);
    if (ssFile.ext == COMMMIT_LOG_FILE_EXT) {
      // Replay commit log to populate memtable.
      auto tableIt = memtables_.emplace(
        ssFile.tableName, 
        std::make_unique<MemTable>(ssFile.tableName)
      ).first;
      auto logIt = commitlogs_.emplace(
        ssFile.tableName, 
        std::make_unique<CommitLog>(dir.c_str(), ssFile.tableName)
      ).first;
      logIt->second->Replay(tableIt->second.get());
    } else if (ssFile.ext == SS_INDEX_FILE_EXT) {
      PutSSIndexToMap(
        ssFile.tableName, 
        ssFile.tableName + "-" + ssFile.uuid,
        std::make_unique<SSIndex>(
          dir_ + "/tabula-data", 
          ssFile.tableName + "-" + ssFile.uuid
        )
      );
    }
    entry = readdir(dirPtr);
  }
  closedir(dirPtr);
}

void Tabula::Flush(MemTable* memtable) {
  string uniqueFileName = MakeUniqueFileName(memtable->Name());
  std::unique_ptr<SSIndex> ssIndex = std::make_unique<SSIndex>(
    dir_ + "/tabula-data", 
    uniqueFileName
  );
  memtable->Flush(
    dir_ + "/tabula-data",
    uniqueFileName, 
    16,
    ssIndex.get()
  );
  auto commitLogIt = commitlogs_.find(memtable->Name());
  if (commitLogIt == commitlogs_.end()) {
    return;
  }
  commitLogIt->second->Clear();
  ssIndex->Flush();
  PutSSIndexToMap(memtable->Name(), uniqueFileName, std::move(ssIndex));
}

string Tabula::MakeUniqueFileName(const string& tableName) {
  return tableName + "-" + string(Utils::GetTime());
}

void Tabula::PutSSIndexToMap(
  const string& tableName, 
  const string& uniqueFileName, 
  std::unique_ptr<SSIndex> ssIndex
) {
  ssIndex->MemoizeStartAndEndRowFromFile();
  auto tableIt = ssIndices_.find(tableName);
  if (tableIt == ssIndices_.end()) {
    tableIt = ssIndices_.emplace(
      tableName, 
      std::make_unique<std::map<string, std::unique_ptr<SSIndex> > >()
    ).first;
  }
  tableIt->second->insert({uniqueFileName, std::move(ssIndex)});
}

void Tabula::ParseSSFileName(const char* fileName, SSFile* ssFile) {
  // Parse fileName into xxx-yyyy.zzz.
  // Must receive a null terminated string 
  size_t offsetHyphen;
  size_t offsetDot;
  size_t offset = 0;
  for (const char* trv = fileName; *trv; trv++) {
    if (*trv == '-') {
      offsetHyphen = offset;
    } else if (*trv == '.') {
      offsetDot = offset;
    }
    offset++;
  }
  ssFile->tableName = string(fileName, offsetHyphen);
  ssFile->uuid = string(fileName + offsetHyphen + 1, offsetDot - offsetHyphen - 1); // exclude hyphen
  ssFile->ext = string(fileName + offsetDot); // include dot
}

bool Tabula::isValidTableName(const std::string& tableName) {
  size_t dot = tableName.find(".");
  if (dot != string::npos) {
    return false;
  }
  size_t hypehn = tableName.find("-");
  if (hypehn != string::npos) {
    return false;
  }
  return true;
}

int Tabula::GetFromDisk(
  const std::string& tab, 
  const std::string& row, 
  const std::string& col, 
  std::string* val
) {
  auto ssIndexIt = ssIndices_.find(tab);
  if (ssIndexIt == ssIndices_.end()) {
    return NOT_FOUND;
  }
  std::map<std::string, std::unique_ptr<SSIndex> >* ssindexSet = ssIndexIt->second.get();
  std::unique_ptr<Row> selectedRow;
  for (auto it = ssindexSet->begin(); it != ssindexSet->end(); it++) {
    uint64_t offset;
    if (it->second->GetOffset(row, &offset) == SUCCESS) {
      // Read row into memory.
      // Keep the row with the latest update time.
      std::unique_ptr<Row> candidateRow = ReadRowFromSSTable(it->first + SS_INDEX_FILE_EXT, offset);
      if (selectedRow.get() == nullptr) {
        selectedRow = std::move(candidateRow);
      } else if (candidateRow->LastUpdateTime() > selectedRow->LastUpdateTime()) {
        selectedRow = std::move(candidateRow);
      }
    }
  }
  if (selectedRow.get() == nullptr) {
    // If row is still nullptr, row does not exist in the table
    return NOT_FOUND;
  }
  return selectedRow->Get(col, val);

}

std::unique_ptr<Row> Tabula::ReadRowFromSSTable(
  const string& fileName,
  uint64_t offset
) {
  string path = dir_ + "/" + fileName;
  int fd = open(path.c_str(), O_RDONLY);
  return Row::Deserialize(fd, offset); 
}

} // namespace KVStore