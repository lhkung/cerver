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

CommitLog::CommitLog(const std::string& tablename, const std::string& store_dir) : store_dir_(store_dir) {
  mkdir(store_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  string path = store_dir + "/" + tablename + ".commitlog";
  logfd_ = open(path.c_str(), O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
}

CommitLog::CommitLog(const char* dir, char* logname) {
  string path = string(dir) + "/" + string(logname);
  logfd_ = open(path.c_str(), O_RDWR | O_APPEND);
}

CommitLog::~CommitLog() {
  close(logfd_);
}

int CommitLog::LogPut(const std::string& row, const std::string& col, const std::string& val) {
	CommitMetaData metadata;
	metadata.row_len = row.length();
	metadata.col_len = col.length();
	metadata.operation = PUT;
	metadata.val_len = val.length();
  string content = row + col + val;
	write(logfd_, &metadata, sizeof(metadata));
	write(logfd_, content.c_str(), content.length());
	return 0;
}

int CommitLog::LogDelete(const std::string &row, const std::string &col) {
	CommitMetaData metadata;
	metadata.row_len = row.length();
	metadata.col_len = col.length();
	metadata.operation = DELETE;
	metadata.val_len = 6;
	string content = row + col + "DELETE";
	write(logfd_, &metadata, sizeof(metadata));
  write(logfd_, content.c_str(), content.length());
	return 0;
}

int CommitLog::ReadNextCommit(string* row,
                              string* col, 
                              string* val) {
  CommitMetaData metadata;                            
  unsigned long bytes_read = read(logfd_, &metadata, sizeof(metadata));
	if (bytes_read < sizeof(metadata) || bytes_read <= 0) {
		return -1;
	}
  vector<char> buf = vector<char>(metadata.row_len);
  bytes_read = read(logfd_, buf.data(), metadata.row_len);
  if (bytes_read < metadata.row_len) {
		return -1;
	}
  *row = string(buf.data(), metadata.row_len);
  buf = vector<char>(metadata.col_len);
  bytes_read = read(logfd_, buf.data(), metadata.col_len);
  if (bytes_read < metadata.col_len) {
		return -1;
	}
  *col = string(buf.data(), metadata.col_len);
  buf = vector<char>(metadata.val_len);
  bytes_read = read(logfd_, buf.data(), metadata.val_len);
  if (bytes_read < metadata.val_len) {
	  return -1;
  }
  *val = string(buf.data(), metadata.val_len);
  return metadata.operation;
}

void CommitLog::Replay(Table* memtable) {
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
