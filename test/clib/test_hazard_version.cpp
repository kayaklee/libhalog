// Libhalog
// Author: likai.root@gmail.com

#include "clib/hal_hazard_version.h"
#include <gtest/gtest.h>

using namespace libhalog;
using namespace libhalog::clib;

class SimpleObject : public HALHazardNodeI {
  public:
    SimpleObject(int64_t &counter) : counter_(counter) {
      __sync_add_and_fetch(&counter_, 1);
    }
    ~SimpleObject() {};
  public:
    void retire() {
      __sync_add_and_fetch(&counter_, -1);
      delete this;
    }
  private:
    int64_t &counter_;
};

TEST(HALHazardVersion, simple) {
  HALHazardVersion hv;
  int64_t counter = 0;

  uint64_t handle = 0;
  int ret = hv.acquire(handle);
  EXPECT_EQ(HAL_SUCCESS, ret);
  for (int64_t i = 0; i < 64; i++) {
    int ret = hv.add_node(new SimpleObject(counter));
    EXPECT_EQ(HAL_SUCCESS, ret);
    EXPECT_EQ(i+1, counter);
  }
  hv.retire();
  EXPECT_EQ(64, counter);
  hv.release(handle);
  hv.retire();
  EXPECT_EQ(0, counter);

  for (int64_t i = 0; i < 32; i++) {
    int ret = hv.add_node(new SimpleObject(counter));
    EXPECT_EQ(HAL_SUCCESS, ret);
    EXPECT_EQ(i+1, counter);
  }
  ret = hv.acquire(handle);
  EXPECT_EQ(HAL_SUCCESS, ret);
  for (int64_t i = 32; i < 64; i++) {
    int ret = hv.add_node(new SimpleObject(counter));
    EXPECT_EQ(HAL_SUCCESS, ret);
    EXPECT_EQ(i+1, counter);
  }
  hv.retire();
  EXPECT_EQ(32, counter);
  hv.release(handle);
  hv.retire();
  EXPECT_EQ(0, counter);

  ret = hv.acquire(handle);
  EXPECT_EQ(HAL_SUCCESS, ret);
  for (int64_t i = 0; i < 64; i++) {
    int ret = hv.add_node(new SimpleObject(counter));
    EXPECT_EQ(HAL_SUCCESS, ret);
    EXPECT_EQ(i+1, counter);
  }
  EXPECT_EQ(64, counter);
  SimpleObject *node = new SimpleObject(counter);
  ret = hv.add_node(node);
  EXPECT_EQ(HAL_OUT_OF_RESOURCES, ret);
  node->retire();
  hv.release(handle);
  EXPECT_EQ(64, counter);
  ret = hv.add_node(new SimpleObject(counter));
  EXPECT_EQ(HAL_SUCCESS, ret);
  EXPECT_EQ(1, counter);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
