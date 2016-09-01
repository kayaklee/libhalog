// Libhalog
// Author: likai.root@gmail.com

#include "clib/hal_btree.h"
#include <gtest/gtest.h>

using namespace libhalog;
using namespace libhalog::clib;

TEST(BaseNode, search) {
  btree::BaseNode<int64_t, int64_t, 32> node;

  int64_t pos = 0;
  bool found = false;
  int ret = node.search(1000, pos, found);
  EXPECT_EQ(HAL_SUCCESS, ret);
  EXPECT_EQ(0, pos);
  EXPECT_FALSE(found);

  for (int64_t i = 0; i < 32; i++) {
    node.kvs_[i].key_ = i * 2;
  }

  node.size_ = 32;
  for (int64_t i = 0; i < 32; i++) {
    int64_t pos = 0;
    bool found = false;
    int ret = node.search(i * 2 + 1, pos, found);
    EXPECT_EQ(HAL_SUCCESS, ret);
    EXPECT_EQ(i + 1, pos);
    EXPECT_FALSE(found);

    ret = node.search(i * 2, pos, found);
    EXPECT_EQ(HAL_SUCCESS, ret);
    EXPECT_EQ(i, pos);
    EXPECT_TRUE(found);
  }
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
