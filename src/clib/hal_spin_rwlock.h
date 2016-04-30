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
    union Atomic {
      uint64_t v_;
      struct {
        uint64_t r_ref_cnt_:62;
        uint64_t w_pending_:1;
        uint64_t w_lock_flag_:1;
      };
    };
    public:
      HALSpinRWLock();
      ~HALSpinRWLock();
    public:
      bool TryRLock();
      void RLock();
      void RUnlock();
      bool TryLock();
      void Lock();
      void Unlock();
    private:
      volatile Atomic atomic_;
      int64_t w_owner_;
  };

}
}

#endif // __HAL_CLIB_SPIN_RWLOCK_H__
