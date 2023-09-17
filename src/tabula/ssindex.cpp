#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include "ssindex.h"

using std::vector;
using std::string;

namespace KVStore {

SSIndex::SSIndex() {}

SSIndex::SSIndex(const std::string& dir, const std::string& uniqueFileName) 
: indexFilePath_(dir + "/" + uniqueFileName + SS_INDEX_FILE_EXT) {}
SSIndex::~SSIndex() {}

int SSIndex::LoadIndexFileToMemory() {
  int fd = open(indexFilePath_.c_str(), O_RDONLY);
  vector<char> buf;
  MemoizeStartAndEndRowFromFile(fd);
  while (true) {
    ssize_t bytesRead = read(fd, buf.data(), ROW_INDEX_METADATA_LENGTH);
    if (bytesRead < ROW_INDEX_METADATA_LENGTH || bytesRead <= 0) {
      break;
    }
    uint32_t rowNameLen = *reinterpret_cast<uint32_t*>(buf.data());
    uint64_t offset = *reinterpret_cast<uint64_t*>(buf.data() + sizeof(rowNameLen));
    buf = vector<char>(rowNameLen);
    bytesRead = read(fd, buf.data(), rowNameLen);
    if (bytesRead < rowNameLen || bytesRead <= 0) {
      break;
    }
    index_.emplace(
      string(buf.data(), rowNameLen),
      offset
    );
  }
  close(fd);
  loadedToMemory_ = true;
}

void SSIndex::AddIndex(const std::string& row, uint64_t offset) {
  index_.emplace(row, offset);
}

void SSIndex::Flush() {
  MemoizeStartAndEndRowFromMap();
  int indexFd = open(indexFilePath_.c_str(), O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
  uint32_t startRowNameLen = index_.begin()->first.length();
  uint32_t endRowNameLen = (index_.end()--)->first.length();
  write(indexFd, &startRowNameLen, sizeof(startRowNameLen));
  write(indexFd, &endRowNameLen, sizeof(endRowNameLen));
  write(indexFd, &(index_.begin()->first), startRowNameLen);
  write(indexFd, &((index_.end()--)->first), endRowNameLen);
  for (auto it = index_.begin(); it != index_.end(); it++) {
    uint32_t rowNameLen = static_cast<uint32_t>(it->first.length());
    write(indexFd, &rowNameLen, sizeof(rowNameLen));
    write(indexFd, it->first.c_str(), rowNameLen);
    write(indexFd, &it->second, sizeof(uint64_t));
  }
  close(indexFd);
  Clear();
}

void SSIndex::Clear() {
  index_.clear();
  loadedToMemory_ = false;
}

void SSIndex::MemoizeStartAndEndRowFromMap() {
  startRow_ = index_.begin()->first;
  endRow_ = (index_.end()--)->first;
}

void SSIndex::MemoizeStartAndEndRowFromFile(int fd) {
  vector<char> buf = vector<char>(START_AND_END_ROW_METADATA_LENGTH);
  ssize_t bytesRead = read(fd, buf.data(), START_AND_END_ROW_METADATA_LENGTH);
  if (bytesRead < START_AND_END_ROW_METADATA_LENGTH || bytesRead <= 0) {
    return;
  }
  uint32_t startRowNameLen = *reinterpret_cast<uint32_t*>(buf.data());
  uint32_t endRowNameLen = *reinterpret_cast<uint32_t*>(buf.data() + sizeof(startRowNameLen)); 
  buf = vector<char>(startRowNameLen + endRowNameLen);
  bytesRead = read(fd, buf.data(), startRowNameLen + endRowNameLen);
  startRow_ = string(buf.data(), startRowNameLen);
  endRow_ = string(buf.data() + startRowNameLen, endRowNameLen);
}

void SSIndex::MemoizeStartAndEndRowFromFile() {
  int indexFd = open(indexFilePath_.c_str(), O_RDONLY);
  MemoizeStartAndEndRowFromFile(indexFd);
  close(indexFd);
}

int SSIndex::GetOffset(const std::string& row, uint64_t* offset) {
  if (row.compare(startRow_) < 0 || row.compare(endRow_) > 0) {
    // If the row is not between start and end row, it definitely
    // does not exist in this index. 
    return NOT_FOUND;
  }
  if (!loadedToMemory_) {
    LoadIndexFileToMemory();
  }
  auto it = index_.find(row);
  if (it == index_.end()) {
    return NOT_FOUND;
  }
  *offset = it->second;
  return SUCCESS;
}

} // namespace KVStore