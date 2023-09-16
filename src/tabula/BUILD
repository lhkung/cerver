cc_library(
  name = "row",
  srcs = ["row.cpp"],
  hdrs = ["row.h"],
  visibility = ["//visibility:public"],
)
cc_library(
  name = "ssindex",
  srcs = ["ssindex.cpp"],
  hdrs = ["ssindex.h"],
  visibility = ["//visibility:public"],
)
cc_library(
  name = "memtable",
  srcs = ["memtable.cpp"],
  hdrs = ["memtable.h"],
  deps = [":row", ":ssindex"],
  visibility = ["//visibility:public"],
)
cc_library(
  name = "commitlog",
  srcs = ["commitlog.cpp"],
  hdrs = ["commitlog.h"],
  deps = [":memtable"],
  visibility = ["//visibility:public"],
)
cc_library(
  name = "tabula",
  srcs = ["tabula.cpp"],
  hdrs = ["tabula.h"],
  deps = [":memtable", ":commitlog", "//src:utils"],
  visibility = ["//visibility:public"],
)
cc_test(
  name = "row_test",
  size = "small",
  srcs = ["row_test.cpp"],
  deps = ["@com_google_googletest//:gtest_main", ":row"],
    visibility = ["//visibility:public"],
)
cc_test(
  name = "tabula_test",
  size = "small",
  srcs = ["tabula_test.cpp"],
  deps = ["@com_google_googletest//:gtest_main", ":tabula"],
    visibility = ["//visibility:public"],
)