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
  while (true) {
    vector<char> buf = vector<char>(12);
    ssize_t bytesRead = read(fd, buf.data(), 12);
    if (bytesRead < 12 || bytesRead <= 0) {
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
  loadedToMemory_ = true;
}

void SSIndex::AddIndex(const std::string& row, uint64_t offset) {
  index_.emplace(row, offset);
}

void SSIndex::Flush() {
  int indexFd = open(indexFilePath_.c_str(), O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
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

} // namespace KVStore