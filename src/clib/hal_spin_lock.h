// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_SPIN_LOCK_H__
#define __HAL_CLIB_SPIN_LOCK_H__
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "hal_util.h"

namespace libhalog {
namespace clib {

  class HALSpinLock {
    public:
      HALSpinLock() : atomic_(0) {}
      ~HALSpinLock() {}
    public:
      bool try_lock() {
        bool bret = false;
        if (0 == ATOMIC_LOAD(&atomic_)
            && __sync_bool_compare_and_swap(&atomic_, 0, 1)) {
          bret = true;
        }
        return bret;
      }
      void lock() {
        while (!(0 == ATOMIC_LOAD(&atomic_)
              && __sync_bool_compare_and_swap(&atomic_, 0, 1))) {
          PAUSE();
        }
      }
      void unlock() {
        assert(1 == ATOMIC_LOAD(&atomic_)
            && __sync_bool_compare_and_swap(&atomic_, 1, 0));
      }
    private:
      int8_t atomic_;
  };

  class HALLockGuard {
    public:
      HALLockGuard() : lock_(NULL) {}
      ~HALLockGuard() {
        if (NULL != lock_) {
          lock_->unlock();
          lock_ = NULL;
        }
      }
      void set_lock(HALSpinLock *lock) {lock_ = lock;}
    private:
      HALSpinLock *lock_;
  };

}
}

#endif // __HAL_CLIB_SPIN_LOCK_H__
