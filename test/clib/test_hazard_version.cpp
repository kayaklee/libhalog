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

  // test entire retire
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

  // test partial retire
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

  // test acquire in one thread over limit
  for (int64_t n = 0; n < 2; n++) {
    uint64_t handles[16];
    for (int64_t i = 0; i < 16; i++) {
      int ret = hv.acquire(handles[i]);
      EXPECT_EQ(HAL_SUCCESS, ret);
      printf("%lx\n", handles[i]);
    }
    int ret = hv.acquire(handle);
    EXPECT_EQ(HAL_OUT_OF_RESOURCES, ret);
    printf("%lx\n", handle);
    for (int64_t i = 0; i < 16; i++) {
      hv.release(handles[i]);
      hv.release(handles[i]);
    }
  }
}

TEST(HALHazardVersion, cc) {
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
