// Libhalog
// Author: likai.root@gmail.com

#include "clib/hal_spin_rwlock.h"
#include "clib/hal_util.h"
#include <gtest/gtest.h>

using namespace libhalog::clib;

struct ThreadTask {
  int64_t count;
  HALSpinRWLock *rwlock;
  volatile int64_t *gdata;
  volatile int64_t *gcheck;
};

typedef void*(*thread_func_pt)(void*);

void *rlock_func(void *data) {
  ThreadTask *tt = (ThreadTask*)(data);
  for (int64_t i = 0; i < tt->count; i++) {
    tt->rwlock->rlock();
    *tt->gcheck = *tt->gdata;
    tt->rwlock->unrlock();
  }
  return NULL;
}

void *try_rlock_func(void *data) {
  ThreadTask *tt = (ThreadTask*)(data);
  for (int64_t i = 0; i < tt->count;) {
    if (tt->rwlock->try_rlock()) {
      *tt->gcheck = *tt->gdata;
      tt->rwlock->unrlock();
      i++;
    }
  }
  return NULL;
}

void *lock_func(void *data) {
  ThreadTask *tt = (ThreadTask*)(data);
  for (int64_t i = 0; i < tt->count; i++) {
    tt->rwlock->lock();
    EXPECT_EQ(*tt->gdata, *tt->gcheck);
    (*tt->gdata)++;
    *tt->gcheck = *tt->gdata;
    tt->rwlock->unlock();
  }
  return NULL;
}

void *try_lock_func(void *data) {
  ThreadTask *tt = (ThreadTask*)(data);
  for (int64_t i = 0; i < tt->count;) {
    if (tt->rwlock->try_lock()) {
      EXPECT_EQ(*tt->gdata, *tt->gcheck);
      (*tt->gdata)++;
      *tt->gcheck = *tt->gdata;
      tt->rwlock->unlock();
      i++;
    }
  }
  return NULL;
}

struct ThreadFunc {
  thread_func_pt thread_func;
  int64_t thread_count;
};

typedef ThreadFunc ThreadFuncArray[4];

void test(
    const int64_t count_per_thread,
    const int64_t rlock_thread_count,
    const int64_t try_rlock_thread_count,
    const int64_t lock_thread_count,
    const int64_t try_lock_thread_count) {
  ThreadFuncArray thread_func_array;
  thread_func_array[0].thread_func = rlock_func;
  thread_func_array[0].thread_count = rlock_thread_count;
  thread_func_array[1].thread_func = try_rlock_func;
  thread_func_array[1].thread_count = try_rlock_thread_count;
  thread_func_array[2].thread_func = lock_func;
  thread_func_array[2].thread_count = lock_thread_count;
  thread_func_array[3].thread_func = try_lock_func;
  thread_func_array[3].thread_count = try_lock_thread_count;

  int64_t total_thread_count = rlock_thread_count + try_rlock_thread_count + lock_thread_count + try_lock_thread_count;
  ThreadTask *tt = new ThreadTask[total_thread_count];
  pthread_t *td = new pthread_t[total_thread_count];

  HALSpinRWLock rwlock;
  volatile int64_t gdata = 0;
  volatile int64_t gcheck = 0;
  for (uint64_t i = 0, t = 0; i < ARRAYSIZE(thread_func_array); i++) {
    for (int64_t j = 0; j < thread_func_array[i].thread_count; j++, t++) {
      tt[t].count = count_per_thread;
      tt[t].rwlock = &rwlock;
      tt[t].gdata = &gdata;
      tt[t].gcheck = &gcheck;
      pthread_create(&td[t], NULL, thread_func_array[i].thread_func, &tt[t]);
    }
  }
  for (uint64_t i = 0, t = 0; i < ARRAYSIZE(thread_func_array); i++) {
    for (int64_t j = 0; j < thread_func_array[i].thread_count; j++, t++) {
      pthread_join(td[t], NULL);
    }
  }

  ASSERT_EQ(gdata, gcheck);
  ASSERT_EQ(gdata, (lock_thread_count + try_lock_thread_count) * count_per_thread);

  delete[] td;
  delete[] tt;
}

TEST(HALSpinRWLock, multi_thread) {
  test(10000000, 2, 2, 2, 2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
