// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_UTIL_H__
#define __HAL_CLIB_UTIL_H__

#include <unistd.h>
#include <sys/syscall.h>

#define LIKELY(x) __builtin_expect(!!(x),1)
#define UNLIKELY(x) __builtin_expect(!!(x),0)
#define GETTID() clib::gettid()
#define PAUSE() asm("pause\n")

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

}
}

#endif // __HAL_CLIB_UTIL_H__
