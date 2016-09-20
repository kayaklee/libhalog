// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_HAZARD_VERSION_H__
#define __HAL_CLIB_HAZARD_VERSION_H__
#include <stdint.h>

#include "clib/hal_define.h"
#include "clib/hal_error.h"
#include "clib/hal_atomic.h"
#include "clib/hal_base_log.h"
#include "clib/hal_spin_lock.h"

namespace libhalog {
namespace clib {
namespace hazard_version {
  class ThreadStore;
}
  class HALHazardNodeI {
    friend class hazard_version::ThreadStore;
    public:
      HALHazardNodeI() : __next__(NULL), __version__(UINT64_MAX) {}
      virtual ~HALHazardNodeI() {}
    public:
      virtual void retire() = 0;
    private:
      void __set_next__(HALHazardNodeI *next) { assert(this != next); __next__ = next; }
      HALHazardNodeI *__get_next__() const {return __next__;}
      void __set_version__(const uint64_t version) {__version__ = version;}
      uint64_t __get_version__() const {return __version__;}
    private:
      HALHazardNodeI *__next__;
      uint64_t __version__;
  };

namespace hazard_version {
  struct VersionHandle {
    union {
      struct {
        uint16_t tid_;
        uint16_t _;
        uint32_t seq_;
      };
      uint64_t u64_;
    };
    VersionHandle(const uint64_t uv) : u64_(uv) {};
  };

  class ThreadStore {
    public:
      ThreadStore();
      ~ThreadStore();
    public:
      void set_enabled(const uint16_t tid);
      bool is_enabled() const;
      uint16_t get_tid() const;

      void set_next(ThreadStore *ts);
      ThreadStore *get_next() const;

      int acquire(const uint64_t version, VersionHandle &handle);
      void release(const VersionHandle &handle);

      int add_node(const uint64_t version, HALHazardNodeI *node);
      int64_t get_hazard_waiting_count() const;
      int64_t retire(const uint64_t version, ThreadStore &node_receiver);

      uint64_t get_version() const;
    private:
      void add_nodes_(HALHazardNodeI *head, HALHazardNodeI *tail, const int64_t count);
    private:
      bool enabled_;
      uint16_t tid_;
      uint64_t last_retire_version_;

      struct {
        uint32_t curr_seq_;
        uint64_t curr_version_;
      } CACHE_ALIGNED;

      HALHazardNodeI *hazard_waiting_list_ CACHE_ALIGNED;
      int64_t hazard_waiting_count_ CACHE_ALIGNED;

      ThreadStore *next_ CACHE_ALIGNED;
  };

  class DummyHazardNode : public HALHazardNodeI {
    public:
      void retire() {}
  };
}

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  template <uint16_t MaxThreadCnt>
  class HALHazardVersionT {
    public:
      HALHazardVersionT(
        const int64_t thread_waiting_threshold = 64,
        const int64_t min_version_cache_timeus = 200000);
      ~HALHazardVersionT();
    public:
      int add_node(HALHazardNodeI *node);
      int acquire(uint64_t &handle);
      void release(const uint64_t handle);
      void retire();
      int64_t get_hazard_waiting_count() const;
    private:
      int get_thread_store_(hazard_version::ThreadStore *&ts);
      uint64_t get_min_version_(const bool force_flush);
    private:
      int64_t thread_waiting_threshold_;
      int64_t min_version_cache_timeus_;

      uint64_t version_ CACHE_ALIGNED;

      HALSpinLock thread_lock_ CACHE_ALIGNED;
      hazard_version::ThreadStore threads_[MaxThreadCnt];
      hazard_version::ThreadStore *thread_list_;
      int64_t thread_count_;

      int64_t hazard_waiting_count_ CACHE_ALIGNED;

      struct {
        uint64_t curr_min_version_;
        int64_t curr_min_version_timestamp_;
      } CACHE_ALIGNED;
  };

  typedef HALHazardVersionT<HAL_MAX_THREAD_COUNT> HALHazardVersion;

  ////////////////////////////////////////////////////////////////////////////////////////////////////

namespace hazard_version {
  ThreadStore::ThreadStore() 
    : enabled_(false),
      tid_(0),
      last_retire_version_(0),
      curr_seq_(0),
      curr_version_(UINT64_MAX),
      hazard_waiting_list_(NULL),
      hazard_waiting_count_(0),
      next_(NULL) {
  }

  ThreadStore::~ThreadStore() {
    while (NULL != hazard_waiting_list_) {
      hazard_waiting_list_->retire();
      hazard_waiting_list_ = hazard_waiting_list_->__get_next__();
    }
  }

  void ThreadStore::set_enabled(const uint16_t tid) {
    enabled_ = true;
    tid_ = tid;
  }

  bool ThreadStore::is_enabled() const {
    return enabled_;
  }

  uint16_t  ThreadStore::get_tid() const {
    return tid_;
  }

  void ThreadStore::set_next(ThreadStore *ts) {
    next_ = ts;
  }

  ThreadStore *ThreadStore::get_next() const {
    return next_;
  }

  int ThreadStore::acquire(const uint64_t version, VersionHandle &handle) {
    assert(tid_ == gettn());
    int ret = HAL_SUCCESS;
    if (UINT64_MAX != curr_version_) {
      LOG_WARN(CLIB, "current thread has already assigned a version handle, seq=%u", curr_seq_);
      ret = HAL_EBUSY;
    } else {
      curr_version_ = version;
      handle.tid_ = tid_;
      handle._ = 0;
      handle.seq_ = curr_seq_;
    }
    return ret;
  }

  void ThreadStore::release(const VersionHandle &handle) {
    assert(tid_ == gettn());
    if (tid_ != handle.tid_
        && curr_seq_ != handle.seq_) {
      LOG_WARN(CLIB, "invalid handle, seq=%u tid=%hu", handle.seq_, handle.tid_);
    } else {
      curr_version_ = UINT64_MAX;
      curr_seq_++;
    }
  }

  int ThreadStore::add_node(const uint64_t version, HALHazardNodeI *node) {
    assert(tid_ == gettn());
    int ret = HAL_SUCCESS;
    node->__set_version__(version);
    add_nodes_(node, node, 1);
    return ret;
  }

  int64_t ThreadStore::retire(const uint64_t version, ThreadStore &node_receiver) {
    assert(this != &node_receiver || tid_ == gettn());
    if (last_retire_version_ == version) {
      return 0;
    }
    last_retire_version_ = version;

    HALHazardNodeI *curr = ATOMIC_LOAD(&hazard_waiting_list_);
    HALHazardNodeI *old = curr;
    while (old != (curr = __sync_val_compare_and_swap(&hazard_waiting_list_, old, NULL))) {
      old = curr;
    }

    HALHazardNodeI *list2retire = NULL;
    int64_t move_count = 0;
    int64_t retire_count = 0;
    DummyHazardNode pseudo_head;
    pseudo_head.__set_next__(curr);
    HALHazardNodeI *iter = &pseudo_head;
    while (NULL != iter->__get_next__()) {
      if (iter->__get_next__()->__get_version__() <= version) {
        retire_count++;
        HALHazardNodeI *tmp = iter->__get_next__();
        iter->__set_next__(iter->__get_next__()->__get_next__());

        tmp->__set_next__(list2retire);
        list2retire = tmp;
      } else {
        move_count++;
        iter = iter->__get_next__();
      }
    }

    HALHazardNodeI *move_list_head = NULL;
    HALHazardNodeI *move_list_tail = NULL;
    if (NULL != (move_list_head = pseudo_head.__get_next__())) {
      move_list_tail = iter;
    }
    node_receiver.add_nodes_(move_list_head, move_list_tail, move_count);
    __sync_add_and_fetch(&hazard_waiting_count_, -(move_count+retire_count));

    while (NULL != list2retire) {
      HALHazardNodeI *node2retire = list2retire;
      list2retire = list2retire->__get_next__();
      node2retire->retire();
    }
    return retire_count;
  }

  uint64_t ThreadStore::get_version() const {
    return curr_version_;
  }

  int64_t ThreadStore::get_hazard_waiting_count() const {
    return ATOMIC_LOAD(&hazard_waiting_count_);
  }

  void ThreadStore::add_nodes_(HALHazardNodeI *head, HALHazardNodeI *tail, const int64_t count) {
    // Thread is only one thread add node, no ABA problem
    assert(tid_ == gettn());
    if (0 < count) {
      HALHazardNodeI *curr = ATOMIC_LOAD(&hazard_waiting_list_);
      HALHazardNodeI *old = curr;
      tail->__set_next__(curr);
      while (old != (curr = __sync_val_compare_and_swap(&hazard_waiting_list_, old, head))) {
        old = curr;
        tail->__set_next__(old);
      }
      __sync_add_and_fetch(&hazard_waiting_count_, count);
    }
  }
}

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  template <uint16_t MaxThreadCnt>
  HALHazardVersionT<MaxThreadCnt>::HALHazardVersionT(
    const int64_t thread_waiting_threshold,
    const int64_t min_version_cache_timeus)
    : thread_waiting_threshold_(thread_waiting_threshold),
      min_version_cache_timeus_(min_version_cache_timeus),
      version_(0), 
      thread_lock_(),
      thread_list_(NULL),
      thread_count_(0),
      hazard_waiting_count_(0),
      curr_min_version_(0),
      curr_min_version_timestamp_(0) {
  }

  template <uint16_t MaxThreadCnt>
  HALHazardVersionT<MaxThreadCnt>::~HALHazardVersionT() {
    retire();
  }

  template <uint16_t MaxThreadCnt>
  int HALHazardVersionT<MaxThreadCnt>::add_node(HALHazardNodeI *node) {
    int ret = HAL_SUCCESS;
    hazard_version::ThreadStore *ts = NULL;
    if (NULL == node) {
      LOG_WARN(CLIB, "invalid param, node null pointer");
      ret = HAL_INVALID_PARAM;
    } else if (HAL_SUCCESS != (ret = get_thread_store_(ts))) {
      LOG_WARN(CLIB, "get_thread_store_ fail, ret=%d", ret);
    } else if (HAL_SUCCESS != (ret = ts->add_node(__sync_add_and_fetch(&version_, 1), node))) {
      LOG_WARN(CLIB, "add_node fail, ret=%d", ret);
    } else {
      __sync_add_and_fetch(&hazard_waiting_count_, 1);
    }
    return ret;
  }

  template <uint16_t MaxThreadCnt>
  int HALHazardVersionT<MaxThreadCnt>::acquire(uint64_t &handle) {
    int ret = HAL_SUCCESS;
    hazard_version::ThreadStore *ts = NULL;
    if (HAL_SUCCESS != (ret = get_thread_store_(ts))) {
      LOG_WARN(CLIB, "get_thread_store_ fail, ret=%d", ret);
    } else {
      while (true) {
        const uint64_t version = ATOMIC_LOAD(&version_);
        hazard_version::VersionHandle version_handle(0);
        if (HAL_SUCCESS != (ret = ts->acquire(version, version_handle))){
          LOG_WARN(CLIB, "thread store acquire fail, ret=%d", ret);
          break;
        } else if (version != ATOMIC_LOAD(&version_)) {
          ts->release(version_handle);
        } else {
          handle = version_handle.u64_;
          break;
        }
      }
    }
    return ret;
  }

  template <uint16_t MaxThreadCnt>
  void HALHazardVersionT<MaxThreadCnt>::release(const uint64_t handle) {
    hazard_version::VersionHandle version_handle(handle);
    if (MaxThreadCnt > version_handle.tid_) {
      hazard_version::ThreadStore *ts = &threads_[version_handle.tid_];
      ts->release(version_handle);
      if (thread_waiting_threshold_ < ts->get_hazard_waiting_count()) {
        uint64_t min_version = get_min_version_(false);
        int64_t retire_count = ts->retire(min_version, *ts);
        __sync_add_and_fetch(&hazard_waiting_count_, -retire_count);
      } else if (thread_waiting_threshold_ * ATOMIC_LOAD(&thread_count_) < ATOMIC_LOAD(&hazard_waiting_count_)) {
        retire();
      }
      //LOG_DEBUG(CLIB, "ts=%ld hazard_waiting_count=%ld", ts->get_hazard_waiting_count(), hazard_waiting_count_);
    }
  }

  template <uint16_t MaxThreadCnt>
  int HALHazardVersionT<MaxThreadCnt>::get_thread_store_(hazard_version::ThreadStore *&ts) {
    int ret = HAL_SUCCESS;
    uint16_t tn = (uint16_t)(gettn());
    if (MaxThreadCnt <= tn) {
      LOG_WARN(CLIB, "thread number overflow, tn=%hu", tn);
      ret = HAL_TOO_MANY_THREADS;
    } else {
      ts = &threads_[tn];
      if (!ts->is_enabled()) {
        thread_lock_.lock();
        if (!ts->is_enabled()) {
          ts->set_enabled(tn);
          ts->set_next(ATOMIC_LOAD(&thread_list_));
          ATOMIC_STORE(&thread_list_, ts);
          __sync_add_and_fetch(&thread_count_, 1);
        }
        thread_lock_.unlock();
      }
    }
    return ret;
  }

  template <uint16_t MaxThreadCnt>
  uint64_t HALHazardVersionT<MaxThreadCnt>::get_min_version_(const bool force_flush) {
    uint64_t ret = 0;
    if (!force_flush
        && 0 != (ret = ATOMIC_LOAD(&curr_min_version_))
        && (ATOMIC_LOAD(&curr_min_version_timestamp_) + min_version_cache_timeus_) > get_cur_microseconds_time()) {
      // from cache
    } else {
      ret = ATOMIC_LOAD(&version_);
      hazard_version::ThreadStore *iter = ATOMIC_LOAD(&thread_list_);
      while (NULL != iter) {
        uint64_t ts_min_version = iter->get_version();
        if (ret > ts_min_version) {
          ret = ts_min_version;
        }
        iter = iter->get_next();
      }
      ATOMIC_STORE(&curr_min_version_, ret);
      ATOMIC_STORE(&curr_min_version_timestamp_, get_cur_microseconds_time());
    }
    //LOG_DEBUG(CLIB, "get_min_version_=%lu", ret);
    return ret;
  }

  template <uint16_t MaxThreadCnt>
  void HALHazardVersionT<MaxThreadCnt>::retire() {
    int ret = HAL_SUCCESS;
    hazard_version::ThreadStore *ts = NULL;
    if (HAL_SUCCESS != (ret = get_thread_store_(ts))) {
      LOG_WARN(CLIB, "get_thread_store_ fail, ret=%d", ret);
    } else {
      uint64_t min_version = get_min_version_(true);

      int64_t retire_count = ts->retire(min_version, *ts);
      __sync_add_and_fetch(&hazard_waiting_count_, -retire_count);

      hazard_version::ThreadStore *iter = ATOMIC_LOAD(&thread_list_);
      while (NULL != iter) {
        if (iter != ts) {
          int64_t retire_count = iter->retire(min_version, *ts);
          __sync_add_and_fetch(&hazard_waiting_count_, -retire_count);
        }
        iter = iter->get_next();
      }
    }
  }

  template <uint16_t MaxThreadCnt>
  int64_t HALHazardVersionT<MaxThreadCnt>::get_hazard_waiting_count() const {
    return ATOMIC_LOAD(&hazard_waiting_count_);
  }
}
}

#endif // __HAL_CLIB_HAZARD_VERSION_H__
