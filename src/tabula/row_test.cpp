#include <gtest/gtest.h>
#include <string>
#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "row.h"

namespace KVStore {

class RowTest: public ::testing::Test {
public:
	void SetUp() {
		row = new Row("test-row");
		pipe(fd);
	}
	void TearDown() {
		delete row;
	}
	Row* row;
	int fd[2];
};

TEST_F(RowTest, TestPutGet) {
  ASSERT_EQ(SUCCESS, row->Put("col1", "val1"));
  ASSERT_EQ(SUCCESS, row->Put("col2", "val2"));
  ASSERT_EQ(SUCCESS, row->Put("col3", "val3"));
  ASSERT_EQ(SUCCESS, row->Put("col15", "val15"));
  ASSERT_EQ(OVERWRITE, row->Put("col15", "val16"));
  std::string actual;
  ASSERT_EQ(SUCCESS, row->Get("col1", &actual));
  ASSERT_EQ("val1", actual);
  ASSERT_EQ(SUCCESS, row->Get("col2", &actual));
  ASSERT_EQ("val2", actual);
  ASSERT_EQ(SUCCESS, row->Get("col3", &actual));
  ASSERT_EQ("val3", actual);
  ASSERT_EQ(SUCCESS, row->Get("col15", &actual));
  ASSERT_EQ("val16", actual);
  ASSERT_EQ(NOT_FOUND, row->Get("col18", &actual));
}

TEST_F(RowTest, TestDelete) {
  row->Put("col1", "val1");
  row->Put("col2", "val2");
  row->Put("col3", "val3");
  ASSERT_EQ(SUCCESS, row->Delete("col1"));
  std::string actual;
  ASSERT_EQ(NOT_FOUND, row->Get("col1", &actual));
  ASSERT_EQ(NOT_FOUND, row->Delete("col4"));
}


TEST_F(RowTest, TestSerielize) {
  row->Put("col1", "val1");
  row->Put("col2", "val2");
  row->Put("col3", "val3");
  row->Put("col15", "val15");
  std::string serialized = row->Serialize();
  write(fd[1], serialized.c_str(), serialized.length());
  close(fd[1]);
  std::unique_ptr<Row> deserialized = Row::Deserialize(fd[0], 0);
  close(fd[0]);
  std::cout << serialized << std::endl;
  ASSERT_NE(nullptr, deserialized.get());
  ASSERT_EQ(row->Name(), deserialized->Name());
  ASSERT_TRUE(*row == *deserialized);
}

} //namepsace Tabula