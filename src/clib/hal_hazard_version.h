// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_HAZARD_VERSION_H__
#define __HAL_CLIB_HAZARD_VERSION_H__
#include <stdint.h>

#include "clib/hal_define.h"
#include "clib/hal_error.h"
#include "clib/hal_atomic.h"
#include "clib/hal_base_log.h"

#define GetMinVersionIntervalUS 1000000
#define ThreadWaitingThreshold  64

namespace libhalog {
namespace clib {
  class HALHazardNodeI {
    public:
      HALHazardNodeI() : next_(NULL), version_(UINT64_MAX) {}
      virtual ~HALHazardNodeI() {}
      virtual void retire() = 0;
    public:
      void set_next(HALHazardNodeI *next) {next_ = next;}
      HALHazardNodeI *get_next() const {return next_;}
      void set_version(const uint64_t version) {version_ = version;}
      uint64_t get_version() const {return version_;}
    private:
      HALHazardNodeI *next_;
      uint64_t version_;
  };

namespace hazard_version {
  struct VersionNode {
    VersionNode *prev_;
    VersionNode *next_;
    uint64_t version_;
    uint32_t seq_;
    uint16_t pos_;
  };

  struct VersionHandle {
    union {
      struct {
        uint16_t tid_;
        uint16_t pos_;
        uint32_t seq_;
      };
      uint64_t u64_;
    };
    VersionHandle(const uint64_t uv) : u64_(uv) {};
  };

  template <typename T>
  void remove_from_dlist(T *node, T *&head);

  template <uint16_t NestSize>
  class ThreadStoreT {
    typedef ThreadStoreT<NestSize> TS;
    public:
      ThreadStoreT();
      ~ThreadStoreT();
    public:
      void set_enabled();
      bool is_enabled() const;

      void set_next(TS *ts);
      TS *get_next() const;

      int acquire(const uint64_t version, VersionHandle &handle);
      void release(const VersionHandle &handle);

      int add_node(const uint64_t version, HALHazardNodeI *node);
      int64_t get_hazard_waiting_count() const;
      int64_t retire(const uint64_t version);

      uint64_t get_min_version();
    private:
      bool enabled_;
      pthread_spinlock_t lock_;

      VersionNode version_nodes_[NestSize];
      VersionNode *version_free_list_;
      VersionNode *version_using_list_;

      HALHazardNodeI *hazard_waiting_list_;
      int64_t hazard_waiting_count_;

      TS *next_;
  };

  class DummyHazardNode : public HALHazardNodeI {
    public:
      void retire() {}
  };
}

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread>
  class HALHazardVersionT {
    typedef hazard_version::ThreadStoreT<MaxNestCntPerThread> TS;
    public:
      HALHazardVersionT();
      ~HALHazardVersionT();
    public:
      int add_node(HALHazardNodeI *node);
      int acquire(uint64_t &handle);
      void release(const uint64_t handle);
      void retire();
    private:
      int get_thread_store_(TS *&ts, uint16_t *tid);
      uint64_t get_min_version_(const bool force_flush);
    private:
      pthread_spinlock_t lock_;
      uint64_t version_;
      TS threads_[MaxThreadCnt];
      TS *thread_list_;
      int64_t thread_count_;
      int64_t hazard_waiting_count_;

      uint64_t curr_min_version_;
      int64_t curr_min_version_timestamp_;
  };

  typedef HALHazardVersionT<HAL_MAX_THREAD_COUNT, 16> HALHazardVersion;

  ////////////////////////////////////////////////////////////////////////////////////////////////////

namespace hazard_version {

  template <typename T>
  void remove_from_dlist(T *node, T *&head) {
    if (node == head) {
      head = node->next_;
    }
    if (NULL != node->prev_) {
      node->prev_->next_ = node->next_;
    }
    if (NULL != node->next_) {
      node->next_->prev_ = node->prev_;
    }
  }

  template <uint16_t NestSize>
  ThreadStoreT<NestSize>::ThreadStoreT() 
    : enabled_(false),
      version_free_list_(NULL),
      version_using_list_(NULL),
      hazard_waiting_list_(NULL),
      hazard_waiting_count_(0),
      next_(NULL) {
    pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
    memset(version_nodes_, 0, sizeof(version_nodes_));
    for (uint16_t i = 0; i < NestSize; i++) {
      version_nodes_[i].next_ = version_free_list_;
      version_nodes_[i].pos_ = i;
      version_free_list_ = &version_nodes_[i];
    }
  }

  template <uint16_t NestSize>
  ThreadStoreT<NestSize>::~ThreadStoreT() {
    while (NULL != hazard_waiting_list_) {
      hazard_waiting_list_->retire();
      hazard_waiting_list_ = hazard_waiting_list_->get_next();
    }
    pthread_spin_destroy(&lock_);
  }

  template <uint16_t NestSize>
  void ThreadStoreT<NestSize>::set_enabled() {
    enabled_ = true;
  }

  template <uint16_t NestSize>
  bool ThreadStoreT<NestSize>::is_enabled() const {
    return enabled_;
  }

  template <uint16_t NestSize>
  void ThreadStoreT<NestSize>::set_next(TS *ts) {
    next_ = ts;
  }

  template <uint16_t NestSize>
  ThreadStoreT<NestSize> *ThreadStoreT<NestSize>::get_next() const {
    return next_;
  }

  template <uint16_t NestSize>
  int ThreadStoreT<NestSize>::acquire(const uint64_t version, VersionHandle &handle) {
    int ret = HAL_SUCCESS;
    pthread_spin_lock(&lock_);
    if (NULL == version_free_list_) {
      LOG_WARN(CLIB, "no version node, maybe nest too much");
      ret = HAL_OUT_OF_RESOURCES;
    } else {
      VersionNode *version_node = version_free_list_;
      version_free_list_ = version_free_list_->next_;

      version_node->version_ = version;

      version_node->prev_ = NULL;
      version_node->next_ = version_using_list_;
      if (NULL != version_using_list_) {
        version_using_list_->prev_ = version_node;
      }
      version_using_list_ = version_node;

      handle.pos_ = version_node->pos_;
      handle.seq_ = version_node->seq_;
    }
    pthread_spin_unlock(&lock_);
    return ret;
  }

  template <uint16_t NestSize>
  void ThreadStoreT<NestSize>::release(const VersionHandle &handle) {
    pthread_spin_lock(&lock_);
    if (NestSize <= handle.pos_) {
      LOG_WARN(CLIB, "invalid handle, pos=%hu", handle.pos_);
    } else if (version_nodes_[handle.pos_].seq_ != handle.seq_) {
      LOG_WARN(CLIB, "invalid handle, seq=%u", handle.seq_);
    } else {
      __sync_add_and_fetch(&version_nodes_[handle.pos_].seq_, 1);
      remove_from_dlist(&version_nodes_[handle.pos_], version_using_list_);
      version_nodes_[handle.pos_].next_ = version_free_list_;
      version_free_list_ = &version_nodes_[handle.pos_];
    }
    pthread_spin_unlock(&lock_);
  }

  template <uint16_t NestSize>
  int ThreadStoreT<NestSize>::add_node(const uint64_t version, HALHazardNodeI *node) {
    int ret = HAL_SUCCESS;
    pthread_spin_lock(&lock_);
    node->set_version(version);
    node->set_next(hazard_waiting_list_);
    hazard_waiting_list_ = node;
    hazard_waiting_count_++;
    pthread_spin_unlock(&lock_);
    return ret;
  }

  template <uint16_t NestSize>
  int64_t ThreadStoreT<NestSize>::get_hazard_waiting_count() const {
    return hazard_waiting_count_;
  }

  template <uint16_t NestSize>
  int64_t ThreadStoreT<NestSize>::retire(const uint64_t version) {
    int64_t ret = 0;
    HALHazardNodeI *list2retire = NULL;

    pthread_spin_lock(&lock_);
    DummyHazardNode pseudo_head;
    pseudo_head.set_next(hazard_waiting_list_);
    HALHazardNodeI *iter = &pseudo_head;
    while (NULL != iter->get_next()) {
      if (iter->get_next()->get_version() < version) {
        HALHazardNodeI *tmp = iter->get_next();
        iter->set_next(iter->get_next()->get_next());

        tmp->set_next(list2retire);
        list2retire = tmp;
        hazard_waiting_count_--;
      } else {
        iter = iter->get_next();
      }
    }
    hazard_waiting_list_ = pseudo_head.get_next();
    pthread_spin_unlock(&lock_);

    while (NULL != list2retire) {
      HALHazardNodeI *node2retire = list2retire;
      list2retire = list2retire->get_next();
      node2retire->retire();
      ret++;
    }

    return ret;
  }

  template <uint16_t NestSize>
  uint64_t ThreadStoreT<NestSize>::get_min_version() {
    uint64_t ret = UINT64_MAX;
    pthread_spin_lock(&lock_);
    VersionNode *iter = version_using_list_;
    while (NULL != iter) {
      if (ret > iter->version_) {
        ret = iter->version_;
      }
      iter = iter->next_;
    }
    pthread_spin_unlock(&lock_);
    return ret;
  }
}

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread>
  HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread>::HALHazardVersionT()
    : version_(0), 
      thread_list_(NULL),
      thread_count_(0),
      hazard_waiting_count_(0),
      curr_min_version_(0),
      curr_min_version_timestamp_(0) {
    pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread>
  HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread>::~HALHazardVersionT() {
    pthread_spin_destroy(&lock_);
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread>
  int HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread>::add_node(HALHazardNodeI *node) {
    int ret = HAL_SUCCESS;
    TS *ts = NULL;
    if (NULL == node) {
      LOG_WARN(CLIB, "invalid param, node null pointer");
      ret = HAL_INVALID_PARAM;
    } else if (HAL_SUCCESS != (ret = get_thread_store_(ts, NULL))) {
      LOG_WARN(CLIB, "get_thread_store_ fail, ret=%d", ret);
    } else if (HAL_SUCCESS != (ret = ts->add_node(__sync_fetch_and_add(&version_, 1), node))) {
      LOG_WARN(CLIB, "add_node fail, ret=%d", ret);
    } else {
      __sync_add_and_fetch(&hazard_waiting_count_, 1);
    }
    return ret;
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread>
  int HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread>::acquire(uint64_t &handle) {
    int ret = HAL_SUCCESS;
    TS *ts = NULL;
    hazard_version::VersionHandle version_handle(0);
    if (HAL_SUCCESS != (ret = get_thread_store_(ts, &version_handle.tid_))) {
      LOG_WARN(CLIB, "get_thread_store_ fail, ret=%d", ret);
    } else {
      while (true) {
        const uint64_t version = ATOMIC_LOAD(&version_);
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

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread>
  void HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread>::release(const uint64_t handle) {
    hazard_version::VersionHandle version_handle(handle);
    if (MaxThreadCnt > version_handle.tid_) {
      TS *ts = &threads_[version_handle.tid_];
      ts->release(version_handle);
      if (ThreadWaitingThreshold < ts->get_hazard_waiting_count()) {
        uint64_t min_version = get_min_version_(false);
        int64_t retired_count = ts->retire(min_version);
        __sync_add_and_fetch(&hazard_waiting_count_, -retired_count);
      } else if (ThreadWaitingThreshold < (hazard_waiting_count_ / thread_count_)) {
        retire();
      }
    }
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread>
  int HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread>::get_thread_store_(TS *&ts, uint16_t *tid) {
    int ret = HAL_SUCCESS;
    int64_t tn = gettn();
    if (MaxThreadCnt <= tn) {
      LOG_WARN(CLIB, "thread number overflow, tn=%ld", tn);
      ret = HAL_TOO_MANY_THREADS;
    } else {
      ts = &threads_[tn];
      if (NULL != tid) {
        *tid = (uint16_t)(tn & 0xffff);
      }
      if (!ts->is_enabled()) {
        pthread_spin_lock(&lock_);
        if (!ts->is_enabled()) {
          ts->set_enabled();
          ts->set_next(ATOMIC_LOAD(&thread_list_));
          ATOMIC_STORE(&thread_list_, ts);
          __sync_add_and_fetch(&thread_count_, 1);
        }
        pthread_spin_unlock(&lock_);
      }
    }
    return ret;
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread>
  uint64_t HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread>::get_min_version_(const bool force_flush) {
    uint64_t ret = 0;
    if (!force_flush
        && 0 != (ret = ATOMIC_LOAD(&curr_min_version_))
        && (ATOMIC_LOAD(&curr_min_version_timestamp_) + GetMinVersionIntervalUS) > get_cur_microseconds_time()) {
      // from cache
    } else {
      ret = ATOMIC_LOAD(&version_);
      TS *iter = ATOMIC_LOAD(&thread_list_);
      while (NULL != iter) {
        uint64_t ts_min_version = iter->get_min_version();
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

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread>
  void HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread>::retire() {
    uint64_t min_version = get_min_version_(true);
    TS *iter = ATOMIC_LOAD(&thread_list_);
    while (NULL != iter) {
      int64_t retired_count = iter->retire(min_version);
      __sync_add_and_fetch(&hazard_waiting_count_, -retired_count);
      iter = iter->get_next();
    }
  }
}
}

#endif // __HAL_CLIB_HAZARD_VERSION_H__
