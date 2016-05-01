// Libhalog
// Author: likai.root@gmail.com

#include "clib/hal_base_log.h"
#include <gtest/gtest.h>

using namespace libhalog::clib;

TEST(HALLog, basic) {
  HALLog log;
  log.open_log("./log/test_base_log.log", false, true);
  log.write_log("clib", HALLogLevels::HAL_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, "hello key=[%s]", "world");
}

TEST(HALLog, size2switch) {
  HALLog log;
  log.open_log("./log/test_base_log.log", false, true);
  log.set_max_size(20*1024*1024);
  for (int64_t i = 0; i < 1024*1024; i++) {
    log.write_log("clib", HALLogLevels::HAL_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, "hello world i=%ld", i);
  }
}

void *rename_file(void *data) {
  UNUSED(data);
  usleep(1000000);
  rename("./log/test_base_log.log", "./log/test_base_log.log.renamed");
  return NULL;
}

TEST(HALLog, exist2switch) {
  HALLog log;
  log.open_log("./log/test_base_log.log", false, true);
  log.set_check_file_exist(true);
  pthread_t pd;
  pthread_create(&pd, NULL, rename_file, NULL);
  for (int64_t i = 0; i < 1024*1024; i++) {
    log.write_log("clib", HALLogLevels::HAL_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, "hello world i=%ld", i);
  }
}

TEST(HALLog, mkdir) {
  HALLog log;
  log.open_log("/tmp/libhalog/clib/log/test_base_log.log", false, true);
  log.write_log("clib", HALLogLevels::HAL_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, "hello key=[%s]", "world");
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
