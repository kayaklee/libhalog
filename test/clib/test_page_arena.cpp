// Libhalog
// Author: likai.root@gmail.com

#include "clib/hal_page_arena.h"
#include <gtest/gtest.h>

using namespace libhalog;
using namespace libhalog::clib;

void test_basic_reset(HAL64KPageArena &pa) {
  void *nil = NULL;
  void *buf = pa.alloc(1);
  EXPECT_NE(buf, nil);
  EXPECT_EQ(pa.used(), 1);
  EXPECT_EQ(pa.total(), HAL_NORMAL_ALLOC_PAGE + sizeof(pagearena::NormalPage));
  EXPECT_EQ(pa.count(), 1);

  buf = pa.alloc(HAL_NORMAL_ALLOC_PAGE);
  EXPECT_NE(buf, nil);
  EXPECT_EQ(pa.used(), HAL_NORMAL_ALLOC_PAGE + 1);
  EXPECT_EQ(pa.total(), (HAL_NORMAL_ALLOC_PAGE + sizeof(pagearena::NormalPage)) * 2);
  EXPECT_EQ(pa.count(), 2);

  buf = pa.alloc(HAL_NORMAL_ALLOC_PAGE);
  EXPECT_NE(buf, nil);
  EXPECT_EQ(pa.used(), HAL_NORMAL_ALLOC_PAGE * 2 + 1);
  EXPECT_EQ(pa.total(), (HAL_NORMAL_ALLOC_PAGE + sizeof(pagearena::NormalPage)) * 3);
  EXPECT_EQ(pa.count(), 3);

  buf = pa.alloc(HAL_NORMAL_ALLOC_PAGE + 1);
  EXPECT_NE(buf, nil);
  EXPECT_EQ(pa.used(), HAL_NORMAL_ALLOC_PAGE * 3 + 2);
  EXPECT_EQ(pa.total(),
      HAL_NORMAL_ALLOC_PAGE + 1 + sizeof(pagearena::BigPage) + (HAL_NORMAL_ALLOC_PAGE + sizeof(pagearena::NormalPage)) * 3);
  EXPECT_EQ(pa.count(), 4);
}

void test_basic_reuse(HAL64KPageArena &pa) {
  void *nil = NULL;
  void *buf = pa.alloc(1);
  EXPECT_NE(buf, nil);
  EXPECT_EQ(pa.used(), 1);
  EXPECT_EQ(pa.total(), (HAL_NORMAL_ALLOC_PAGE + sizeof(pagearena::NormalPage)) * 3);
  EXPECT_EQ(pa.count(), 1);

  buf = pa.alloc(HAL_NORMAL_ALLOC_PAGE);
  EXPECT_NE(buf, nil);
  EXPECT_EQ(pa.used(), HAL_NORMAL_ALLOC_PAGE + 1);
  EXPECT_EQ(pa.total(), (HAL_NORMAL_ALLOC_PAGE + sizeof(pagearena::NormalPage)) * 3);
  EXPECT_EQ(pa.count(), 2);

  buf = pa.alloc(HAL_NORMAL_ALLOC_PAGE);
  EXPECT_NE(buf, nil);
  EXPECT_EQ(pa.used(), HAL_NORMAL_ALLOC_PAGE * 2 + 1);
  EXPECT_EQ(pa.total(), (HAL_NORMAL_ALLOC_PAGE + sizeof(pagearena::NormalPage)) * 3);
  EXPECT_EQ(pa.count(), 3);

  buf = pa.alloc(HAL_NORMAL_ALLOC_PAGE + 1);
  EXPECT_NE(buf, nil);
  EXPECT_EQ(pa.used(), HAL_NORMAL_ALLOC_PAGE * 3 + 2);
  EXPECT_EQ(pa.total(),
      HAL_NORMAL_ALLOC_PAGE + 1 + sizeof(pagearena::BigPage) + (HAL_NORMAL_ALLOC_PAGE + sizeof(pagearena::NormalPage)) * 3);
  EXPECT_EQ(pa.count(), 4);
}

TEST(HALLog, basic) {
  HALDefaultAllocator allocator;
  HAL64KPageArena pa(0, allocator);

  test_basic_reset(pa);
  pa.reuse();
  test_basic_reuse(pa);
  pa.reset();
  test_basic_reset(pa);
  pa.reuse();
  test_basic_reuse(pa);
  pa.reset();
  test_basic_reset(pa);

  hal_malloc_print_usage();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
