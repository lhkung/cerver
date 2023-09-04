#ifndef COMMIT_LOG_H_
#define COMMIT_LOG_H_
#define PUT 0
#define DELETE 1

#include <string>
#include "memtable.h"

namespace Cerver {

/*
 CommitLog file format:

 row length    (4 bytes)
 column length (4 bytes)
 operation     (4 bytes)
 value length  (4 bytes)
 row
 column
 value
 */

class CommitLog {
public:
	CommitLog(
    const std::string& tableName,
    const std::string& storeDir
  );
	CommitLog(
    const char* dir,
    char* logName
  );
	~CommitLog();
	int LogPut(
    const std::string& row,
    const std::string& col,
    const std::string& val
  );
	int LogDelete(
    const std::string& row,
    const std::string& col
  );
	struct CommitMetaData {
		uint32_t rowLen;
		uint32_t colLen;
		uint32_t operation;
		uint32_t valLen;
	};
	void Clear();
	void Replay(Table* memtable);
private:
	// Read the next commit from the log file.
	// Returns 0 if commit is properly logged and is a PUT.
	// Returns 1 if commit is properly logged and is a DELETE. In this case, val is undefined.
	// Returns -1 if the end of log is reached.
	int ReadNextCommit(
    std::string* row,
    std::string* col, 
    std::string* val
  );
  std::string storeDir_;
  int logfd_;
};
}
// namespace Cerver

#endif
