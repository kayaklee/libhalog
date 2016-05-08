// Libhalog
// Author: likai.root@gmail.com

#include <assert.h>
#include "hal_util.h"
#include "hal_spin_rwlock.h"

namespace libhalog {
namespace clib {

  HALSpinRWLock::Atomic::Atomic() : v_(0) {}
  HALSpinRWLock::Atomic::Atomic(volatile Atomic &other) : v_(other.v_) {}

  HALSpinRWLock::HALSpinRWLock()
    : atomic_(),
      w_owner_(0) {
  }

  HALSpinRWLock::~HALSpinRWLock() {
  }

  bool HALSpinRWLock::try_rlock() {
    bool bret = false;
    Atomic old_v = atomic_;
    Atomic new_v = old_v;
    new_v.r_ref_cnt_++;
    if (0 == old_v.w_pending_ 
        && 0 == old_v.w_lock_flag_
        && MAX_REF_CNT > old_v.r_ref_cnt_
        && __sync_bool_compare_and_swap(&atomic_.v_, old_v.v_, new_v.v_)) {
      bret = true;
    }
    return bret;
  }

  void HALSpinRWLock::rlock() {
    while (true) {
      Atomic old_v = atomic_;
      Atomic new_v = old_v;
      new_v.r_ref_cnt_++;
      if (0 == old_v.w_pending_ 
          && 0 == old_v.w_lock_flag_
          && MAX_REF_CNT > old_v.r_ref_cnt_
          && __sync_bool_compare_and_swap(&atomic_.v_, old_v.v_, new_v.v_)) {
        break;
      }
      PAUSE();
    } // while
  }

  void HALSpinRWLock::unrlock() {
    while (true) {
      Atomic old_v = atomic_;
      Atomic new_v = old_v;
      new_v.r_ref_cnt_--;
      if (0 != old_v.w_lock_flag_
          || 0 == old_v.r_ref_cnt_
          || MAX_REF_CNT < old_v.r_ref_cnt_) {
        assert(0);
      } else if (__sync_bool_compare_and_swap(&atomic_.v_, old_v.v_, new_v.v_)) {
        break;
      } else {
        PAUSE();
      }
    } // while
  }

  bool HALSpinRWLock::try_lock() {
    bool bret = false;
    Atomic old_v = atomic_;
    Atomic new_v = old_v;
    new_v.w_pending_ = 0;
    new_v.w_lock_flag_ = 1;
    if (0 == old_v.w_lock_flag_
        && 0 == old_v.r_ref_cnt_
        && __sync_bool_compare_and_swap(&atomic_.v_, old_v.v_, new_v.v_)) {
      bret = true;
    }
    return bret;
  }

  void HALSpinRWLock::lock() {
    while (true) {
      Atomic old_v = atomic_;
      Atomic new_v = old_v;
      bool pending = false;
      if (0 != old_v.w_lock_flag_
          || 0 != old_v.r_ref_cnt_) {
        new_v.w_pending_ = 1;
        pending = true;
      } else {
        new_v.w_pending_ = 0;
        new_v.w_lock_flag_ = 1;
      }
      if (__sync_bool_compare_and_swap(&atomic_.v_, old_v.v_, new_v.v_)) {
        if (!pending) {
          w_owner_ = gettid();
          break;
        }
      }
      PAUSE();
    } // while
  }

  void HALSpinRWLock::unlock() {
    while (true) {
      Atomic old_v = atomic_;
      Atomic new_v = old_v;
      new_v.w_lock_flag_ = 0;
      if (0 == old_v.w_lock_flag_
          || 0 != old_v.r_ref_cnt_) {
        assert(0);
      } else if (__sync_bool_compare_and_swap(&atomic_.v_, old_v.v_, new_v.v_)) {
        break;
      } else {
        PAUSE();
      }
    } // while
  }

}
}

