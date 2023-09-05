#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <sys/stat.h>
#include "commitlog.h"

using std::string;
using std::vector;

namespace Cerver {

CommitLog::CommitLog(
  const std::string& tableName, 
  const std::string& storeDir
) : storeDir_(storeDir) {
  mkdir(storeDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  string path = storeDir + "/" + tableName + COMMMIT_LOG_FILE_EXT;
  logfd_ = open(path.c_str(), O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
}

CommitLog::CommitLog(
  const char* dir,
  char* logName
) {
  string path = string(dir) + "/" + string(logName);
  logfd_ = open(path.c_str(), O_RDWR | O_APPEND);
}

CommitLog::~CommitLog() {
  close(logfd_);
}

int CommitLog::LogPut(
  const std::string& row, 
  const std::string& col, 
  const std::string& val
) {
	CommitMetaData metadata;
	metadata.rowLen = row.length();
	metadata.colLen = col.length();
	metadata.operation = PUT;
	metadata.valLen = val.length();
  string content = row + col + val;
	write(logfd_, &metadata, sizeof(metadata));
	write(logfd_, content.c_str(), content.length());
	return 0;
}

int CommitLog::LogDelete(
  const std::string &row,
  const std::string &col
) {
	CommitMetaData metadata;
	metadata.rowLen = row.length();
	metadata.colLen = col.length();
	metadata.operation = DELETE;
	metadata.valLen = 6;
	string content = row + col + "DELETE";
	write(logfd_, &metadata, sizeof(metadata));
  write(logfd_, content.c_str(), content.length());
	return 0;
}

int CommitLog::ReadNextCommit(
  string* row,
  string* col, 
  string* val
) {
  CommitMetaData metadata;                            
  unsigned long bytesRead = read(logfd_, &metadata, sizeof(metadata));
	if (bytesRead < sizeof(metadata) || bytesRead <= 0) {
		return -1;
	}
  vector<char> buf = vector<char>(metadata.rowLen);
  bytesRead = read(logfd_, buf.data(), metadata.rowLen);
  if (bytesRead < metadata.rowLen) {
		return -1;
	}
  *row = string(buf.data(), metadata.rowLen);
  buf = vector<char>(metadata.colLen);
  bytesRead = read(logfd_, buf.data(), metadata.colLen);
  if (bytesRead < metadata.colLen) {
		return -1;
	}
  *col = string(buf.data(), metadata.colLen);
  buf = vector<char>(metadata.valLen);
  bytesRead = read(logfd_, buf.data(), metadata.valLen);
  if (bytesRead < metadata.valLen) {
	  return -1;
  }
  *val = string(buf.data(), metadata.valLen);
  return metadata.operation;
}

void CommitLog::Replay(MemTable* memtable) {
  while (true) {
    string row;
    string col;
    string val;
    int op = ReadNextCommit(&row, &col, &val);
    if (op == PUT) {
      memtable->Put(row, col, val);
    } else if (op == DELETE) {
      memtable->Delete(row, col);
    } else {
      break;
    }
  }
}

void CommitLog::Clear() {
	ftruncate(logfd_, 0);
}

} // namespace Cerver
