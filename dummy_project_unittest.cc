#include "dummy_project.h"
#include "gtest/gtest.h"

TEST(DummyProjectTest, OneIsOne) { EXPECT_EQ(1, 1); }

TEST(DummyProjectTest, NamedConstructor) {
  string name = "foo";
  DProject* dp = new DProject(name);
  EXPECT_STREQ(name.c_str(), dp->getDProjectName().c_str());
  delete dp;
}
