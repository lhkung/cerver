#include <gtest/gtest.h>
#include <string>
#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "tabula.h"

namespace KVStore {

TEST(TabulaTest, TestParseSSFileName) {
  const char* fileName = "table-uuid.sst";
  Tabula::SSFile ssFile;
  Tabula::ParseSSFileName(fileName, &ssFile);
  ASSERT_EQ("table", ssFile.tableName);
  ASSERT_EQ("uuid", ssFile.uuid);
  ASSERT_EQ(".sst", ssFile.ext);
}

} // namespce KVStore
