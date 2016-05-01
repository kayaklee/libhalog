// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_UTIL_H__
#define __HAL_CLIB_UTIL_H__

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include "hal_define.h"

#define LIKELY(x) __builtin_expect(!!(x),1)
#define UNLIKELY(x) __builtin_expect(!!(x),0)
#define PAUSE() asm("pause\n")
#define UNUSED(v) ((void)(v))

#define GETTID() clib::gettid()
#define ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))

namespace libhalog {
namespace clib {

  static inline int64_t gettid()
  {
    static __thread int64_t tid = -1;
    if (UNLIKELY(tid == -1)) {
      tid = static_cast<int64_t>(syscall(__NR_gettid));
    }
    return tid;
  }

  static inline int64_t tv_to_microseconds(const timeval &tp)
  {
    return (((int64_t) tp.tv_sec) * 1000000 + (int64_t) tp.tv_usec);
  }

  static inline int64_t get_cur_microseconds_time(void)
  {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tv_to_microseconds(tp);
  }

  static inline timespec microseconds_to_ts(const int64_t microseconds)
  {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;
    return ts;
  }

  static inline const struct tm *get_cur_tm() {
    static __thread struct tm cur_tms[MAX_TSI_COUNT];
    static __thread int64_t pos = 0;
    time_t t = time(NULL);
    return localtime_r(&t, &cur_tms[pos++ % MAX_TSI_COUNT]);
  }

  static inline void get_cur_tm(struct tm &cur_tm) {
    time_t t = time(NULL);
    localtime_r(&t, &cur_tm);
  }

}
}

#endif // __HAL_CLIB_UTIL_H__
