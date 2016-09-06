// Libhalog
// Author: likai.root@gmail.com

#include <unistd.h>
#include "clib/hal_hazard_version.h"
#include <gtest/gtest.h>

using namespace libhalog;
using namespace libhalog::clib;

class GObject: public HALHazardNodeI {
  public:
    GObject(int64_t &counter) : counter_(counter) {
      __sync_add_and_fetch(&counter_, 1);
      memset(data_, '^', sizeof(data_));
    }
    ~GObject() {
      memset(data_, '$', sizeof(data_));
      __sync_add_and_fetch(&counter_, -1);
    }
    bool operator== (const GObject &o) const {
      return (0 == memcmp(data_, o.data_, sizeof(data_)));
    }
    void retire() {
      delete this;
    }
  private:
    int64_t &counter_;
    char data_[64];
};

TEST(HALHazardVersion, simple) {
  HALHazardVersion hv;
  int64_t counter = 0;

  // test entire retire
  uint64_t handle = 0;
  int ret = hv.acquire(handle);
  EXPECT_EQ(HAL_SUCCESS, ret);
  for (int64_t i = 0; i < 64; i++) {
    int ret = hv.add_node(new GObject(counter));
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
    int ret = hv.add_node(new GObject(counter));
    EXPECT_EQ(HAL_SUCCESS, ret);
    EXPECT_EQ(i+1, counter);
  }
  ret = hv.acquire(handle);
  EXPECT_EQ(HAL_SUCCESS, ret);
  for (int64_t i = 32; i < 64; i++) {
    int ret = hv.add_node(new GObject(counter));
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

struct GConf {
  bool stop;
  int64_t counter;
  int64_t read_loops;
  int64_t write_loops;
  GObject *v;
  HALHazardVersion hv;
};

void *read_thread_func(void *data) {
  GConf *g_conf = (GConf*)data;
  GObject checker(g_conf->counter);
  for (int64_t i = 0; i < g_conf->read_loops; i++) {
    uint64_t handle;
    g_conf->hv.acquire(handle);
    GObject *v = ATOMIC_LOAD(&(g_conf->v));
    assert(*v == checker);
    g_conf->hv.release(handle);
  }
  return NULL;
}

void *write_thread_func(void *data) {
  GConf *g_conf = (GConf*)data;
  GObject checker(g_conf->counter);
  for (int64_t i = 0; i < g_conf->write_loops; i++) {
    GObject *v = new GObject(g_conf->counter);
    GObject *curr = ATOMIC_LOAD(&(g_conf->v));
    GObject *old = curr;
    while (old != (curr = __sync_val_compare_and_swap(&(g_conf->v), curr, v))) {
      old = curr;
    }
    g_conf->hv.add_node(old);
  }
  return NULL;
}

void *debug_thread_func(void *data) {
  GConf *g_conf = (GConf*)data;
  while (!ATOMIC_LOAD(&(g_conf->stop))) {
    printf("hazard_waiting_count=%ld\n", g_conf->hv.get_hazard_waiting_count());
    usleep(1000000);
  }
  return NULL;
}

TEST(HALHazardVersion, cc) {
  GConf g_conf;
  g_conf.stop = false;
  g_conf.counter = 0;
  g_conf.read_loops = 10000000;
  g_conf.write_loops = 10000000;
  g_conf.v = new GObject(g_conf.counter);

  int64_t cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  int64_t read_count = (cpu_count + 1) / 2;
  int64_t write_count = (cpu_count + 1) / 2;
  pthread_t *rpd = new pthread_t[read_count];
  pthread_t *wpd = new pthread_t[write_count];

  pthread_t dpd;
  pthread_create(&dpd, NULL, debug_thread_func, &g_conf);
  for (int64_t i = 0; i < read_count; i++) {
    pthread_create(&rpd[i], NULL, read_thread_func, &g_conf);
  }
  for (int64_t i = 0; i < write_count; i++) {
    pthread_create(&wpd[i], NULL, write_thread_func, &g_conf);
  }
  for (int64_t i = 0; i < read_count; i++) {
    pthread_join(rpd[i], NULL);
  }
  for (int64_t i = 0; i < write_count; i++) {
    pthread_join(wpd[i], NULL);
  }
  ATOMIC_STORE(&(g_conf.stop), true);
  pthread_join(dpd, NULL);

  delete[] wpd;
  delete[] rpd;
  delete g_conf.v;

  printf("counter=%ld\n", g_conf.counter);
  g_conf.hv.retire();
  printf("counter=%ld\n", g_conf.counter);
  assert(0 == g_conf.counter);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
