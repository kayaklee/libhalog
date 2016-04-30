// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_SPIN_RWLOCK_H__
#define __HAL_CLIB_SPIN_RWLOCK_H__
#include <stdint.h>
#include "hal_util.h"

namespace libhalog {
namespace clib {

  class HALSpinRWLock {
    static const uint64_t MAX_REF_CNT = 0x00ffffff;
    struct Atomic {
      union {
        uint64_t v_;
        struct {
          uint64_t r_ref_cnt_:62;
          uint64_t w_pending_:1;
          uint64_t w_lock_flag_:1;
        };
      };
      Atomic();
      Atomic(volatile Atomic &other);
    };
    public:
      HALSpinRWLock();
      ~HALSpinRWLock();
    public:
      bool try_rlock();
      void rlock();
      void unrlock();
      bool try_lock();
      void lock();
      void unlock();
    private:
      volatile Atomic atomic_;
      int64_t w_owner_;
  };

}
}

#endif // __HAL_CLIB_SPIN_RWLOCK_H__