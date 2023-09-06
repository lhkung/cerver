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

std::string Row::Serialize() {
  std::ostringstream ss;
  uint32_t rowNameLen = static_cast<uint32_t>(name_.length());
  uint32_t colSize = static_cast<uint32_t>(columns_.size());
  uint64_t lastUpdated = static_cast<uint64_t>(lastUpdated_);
  ss.write(reinterpret_cast<char*>(&rowNameLen), sizeof(rowNameLen));
  ss.write(reinterpret_cast<char*>(&colSize), sizeof(colSize));
  ss.write(reinterpret_cast<char*>(&lastUpdated), sizeof(lastUpdated));
  ss.write(name_.c_str(), name_.length());
  for (auto it = columns_.begin(); it != columns_.end(); it++) {
    uint32_t colNameLen = it->first.length();
    uint64_t colValLen = it->second.length();
    ss.write(reinterpret_cast<char*>(&colNameLen), sizeof(colNameLen));
    ss.write(reinterpret_cast<char*>(&colValLen), sizeof(colValLen));
    ss.write(it->first.c_str(), it->first.length());
    ss.write(it->second.c_str(), it->second.length());
  }
  return ss.str();
}

std::unique_ptr<Row> Row::Deserialize(int fd, uint64_t offset) {
  lseek(fd, offset, 0);
  RowMetaData metadata;
  vector<char> buf = vector<char>(ROW_METADATA_BYTES);
  ssize_t bytesRead = read(fd, buf.data(), ROW_METADATA_BYTES);
  if (bytesRead != ROW_METADATA_BYTES || bytesRead <= 0) {
    return std::unique_ptr<Row>(nullptr);
	}
  metadata.rowNameLen = *reinterpret_cast<uint32_t*>(buf.data());
  metadata.numCols = *reinterpret_cast<uint32_t*>(buf.data() + sizeof(metadata.rowNameLen));
  metadata.lastUpdated = *reinterpret_cast<uint64_t*>(buf.data() + sizeof(metadata.rowNameLen) + sizeof(metadata.numCols));
  buf = vector<char>(metadata.rowNameLen);
  bytesRead = read(fd, buf.data(), metadata.rowNameLen);
  if (bytesRead != metadata.rowNameLen || bytesRead <= 0) {
    return std::unique_ptr<Row>(nullptr);
	}
  std::unique_ptr<Row> row = std::make_unique<Row>(
    string(buf.data(), metadata.rowNameLen), 
    metadata.lastUpdated
  );
  for (uint32_t i = 0; i < metadata.numCols; i++) {
    buf = vector<char>(COL_METADATA_BYTES);
    bytesRead = read(fd, buf.data(), COL_METADATA_BYTES);
    if (bytesRead < COL_METADATA_BYTES || bytesRead <= 0) {
      return std::unique_ptr<Row>(nullptr);
	  }
    uint32_t colNameLen = *reinterpret_cast<uint32_t*>(buf.data());
    uint64_t colValLen = *reinterpret_cast<uint32_t*>(buf.data() + sizeof(colNameLen));
    buf = vector<char>(colNameLen + colValLen);
    bytesRead = read(fd, buf.data(), colNameLen + colValLen);
    if (bytesRead != colNameLen + colValLen || bytesRead <= 0) {
     return std::unique_ptr<Row>(nullptr);
	  }
    row->PutWithoutUpdateTime(
      string(buf.data(), colNameLen),
      string(buf.data() + colNameLen, colValLen)
    );
  }
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

time_t Row::LastUpdateTime() {
  return lastUpdated_;
}


} // namespace KVStore