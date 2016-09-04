// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_UTIL_H__
#define __HAL_CLIB_UTIL_H__

#include <typeinfo>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include "hal_define.h"
#include "hal_atomic.h"

#define LIKELY(x) __builtin_expect(!!(x),1)
#define UNLIKELY(x) __builtin_expect(!!(x),0)
#define PAUSE() asm("pause\n")
#define UNUSED(v) ((void)(v))

#define ARRAYSIZE(array) ((int64_t)(sizeof(array) / sizeof(array[0])))

namespace libhalog {
namespace clib {

  void log_kv(const char *name, void *ptr);

  static inline int64_t gettid() {
    static __thread int64_t tid = -1;
    if (UNLIKELY(tid == -1)) {
      tid = static_cast<int64_t>(syscall(__NR_gettid));
    }
    return tid;
  }

  static inline int64_t gettn() {
    static int64_t gn = 0;
    static __thread int64_t tn = -1;
    if (-1 == tn) {
      tn = __sync_fetch_and_add(&gn, 1);
    }
    return tn;
  }

  template <typename T>
  T &gsi() {
    static T t;
    static bool once = true;
    if (ATOMIC_LOAD(&once)
        && __sync_val_compare_and_swap(&once, true, false)) {
      log_kv(typeid(T).name(), &t);
    }
    return t;
  }

  template <typename T>
  T *&tsi() {
    static __thread T *t = NULL;
    static bool once = true;
    if (NULL != t
        && ATOMIC_LOAD(&once)
        && __sync_val_compare_and_swap(&once, true, false)) {
      log_kv(typeid(T).name(), t);
    }
    return t;
  }

  template <typename T>
  void set_tsi(T *t) {
    tsi<T>() = t;
  }

  template <typename T>
  void set_tsi(T &t) {
    tsi<T>() = &t;
  }

  template <typename T>
  T *get_tsi() {
    return tsi<T>();
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  static inline int64_t tv_to_microseconds(const timeval &tp) {
    return (((int64_t) tp.tv_sec) * 1000000 + (int64_t) tp.tv_usec);
  }

  static inline timespec microseconds_to_ts(const int64_t microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;
    return ts;
  }

  static inline int64_t get_cur_microseconds_time(void) {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tv_to_microseconds(tp);
  }

  static inline const struct tm *get_cur_tm() {
    static __thread struct tm cur_tms[HAL_MAX_THREAD_COUNT];
    static __thread int64_t pos = 0;
    time_t t = time(NULL);
    return localtime_r(&t, &cur_tms[pos++ % HAL_MAX_THREAD_COUNT]);
  }

  static inline void get_cur_tm(struct tm &cur_tm) {
    time_t t = time(NULL);
    localtime_r(&t, &cur_tm);
  }

}
}

#endif // __HAL_CLIB_UTIL_H__
