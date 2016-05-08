// Libhalog
// Author: likai.root@gmail.com

#include "clib/hal_base_log.h"
#include "clib/hal_util.h"
#include <gtest/gtest.h>

using namespace libhalog;
using namespace libhalog::clib;

TEST(HALLog, basic) {
  HALLog log;
  log.open_log("./log/test_base_log.log", false, true);
  log.write_log("clib", HALLogLevels::HAL_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, "hello key=[%s]", "world");
  SET_TSI_LOGGER(&log);
  LOG_INFO(CLIB, "write by macro, [%s]", "Hello world!");
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

struct ThreadTask {
  int64_t count;
  HALLog *log;
};

void *thread_func(void *data) {
  ThreadTask *tt = (ThreadTask*)data;
  for (int64_t i = 0; i < tt->count; i++) {
    tt->log->write_log("clib", HALLogLevels::HAL_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, "hello world i=%ld", i);
  }
  return NULL;
}

void test(
    const int64_t count_per_thread,
    const int64_t thread_count) {
  HALLog log;
  log.open_log("./log/test_base_log.log", false, true);
  log.set_max_size(100*1024*1024);
  ThreadTask tt;
  tt.count = count_per_thread;
  tt.log = &log;

  pthread_t *td = new pthread_t[thread_count];
  for (int64_t i = 0; i < thread_count; i++) {
    pthread_create(&td[i], NULL, thread_func, &tt);
  }
  for (int64_t i = 0; i < thread_count; i++) {
    pthread_join(td[i], NULL);
  }
  delete[] td;
}

TEST(HALLog, concurrnet) {
  test(1024*1024, 4);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
