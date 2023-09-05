#include <sstream>
#include <unistd.h>
#include <vector>
#include <iostream>
#include "row.h"

using std::string;
using std::vector;

namespace KVStore {

Row::Row(const std::string& name) : Row(name, -1) {}
Row::Row(
  const std::string& name,
  time_t lastUpdated) 
  : name_(name),
    lastUpdated_(lastUpdated) 
{}
Row::~Row() {}

int Row::Put(
  const std::string& col, 
  const std::string& val
) {
  lastUpdated_ = time(0);
  return PutWithoutUpdateTime(col, val);
}

int Row::PutWithoutUpdateTime(
  const std::string& col, 
  const std::string& val
) {
  auto it = columns_.find(col);
  if (it == columns_.end()) {
    it = columns_.emplace(col, val).first;
    return SUCCESS;
  }
  it->second = val;
  return OVERWRITE;
}

int Row::Get(
  const std::string& col, 
  std::string* val
) {
  auto it = columns_.find(col);
  if (it == columns_.end()) {
    return NOT_FOUND;
  }
  *val = it->second;
  return SUCCESS;
}

int Row::Delete(const std::string& col) {
   auto it = columns_.find(col);
   if (it == columns_.end()) {
    return NOT_FOUND;
  }
  lastUpdated_ = time(0);
  columns_.erase(it);
  return SUCCESS;
}

const std::string &Row::Name() {
  return name_;
}

string Row::Serialize() {
  std::ostringstream ss;
  ss << name_.length() << " " << columns_.size() << " " << lastUpdated_ << " " << name_;
  for (auto it = columns_.begin(); it != columns_.end(); it++) {
    ss << it->first.length() << " " << it->second.length() << " " << it->first << it->second;
  }
  return ss.str();
}

std::unique_ptr<Row> Row::Deserialize(int fd, uint64_t offset) {
  FILE* fp = fdopen(fd, "r");
  RowMetaData metadata;
  fscanf(fp, "%d %d %lld ", &metadata.rowNameLen, &metadata.numCols, &metadata.lastUpdated);
  vector<char> buf = vector<char>(metadata.rowNameLen);
  buf = vector<char>(metadata.rowNameLen);
  size_t bytesRead = fread(buf.data(), sizeof(char), metadata.rowNameLen, fp);
  if (bytesRead < metadata.rowNameLen || bytesRead <= 0) {
    return std::unique_ptr<Row>(nullptr);
	}
  std::unique_ptr<Row> row = std::make_unique<Row>(
    string(buf.data(), metadata.rowNameLen), 
    metadata.lastUpdated
  );
  for (uint32_t i = 0 ; i < metadata.numCols; i++) {
    uint32_t colNameLen;
    uint64_t colValLen;
    fscanf(fp, "%d %lld ", &colNameLen, &colValLen);
    buf = vector<char>(colNameLen + colValLen);
    bytesRead = fread(buf.data(), sizeof(char), colNameLen + colValLen, fp);
    if (bytesRead < colNameLen + colValLen || bytesRead <= 0) {
    return std::unique_ptr<Row>(nullptr);
	  }
    row->PutWithoutUpdateTime(
      string(buf.data(), colNameLen),
      string(buf.data() + colNameLen, colValLen)
    );
  }
  fclose(fp);
  return row;
}

bool Row::operator == (const Row& right) const {
  if (name_ != right.name_) {
    return false;
  }
  if (lastUpdated_ != right.lastUpdated_) {
    return false;
  }
  if (columns_.size() != right.columns_.size()) {
    return false;
  }
  for (auto leftIt = columns_.begin(); leftIt != columns_.end(); leftIt ++) {
    auto rightIt = right.columns_.find(leftIt->first);
    if (rightIt == right.columns_.end()) {
      return false;
    }
    if (leftIt->second != rightIt->second) {
      return false;
    }
  }
  return true;
}

bool Row::operator != (const Row& right) const {
  return !operator == (right);
}


} // namespace KVStore