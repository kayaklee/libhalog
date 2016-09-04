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

namespace libhalog {
namespace clib {
  class HALHazardNodeI;

namespace hazard_version {
  struct VersionNode {
    VersionNode *next_;
    uint64_t version_;
    uint32_t seq_;
    uint16_t pos_;
  };

  struct HazardNode {
    HazardNode *next_;
    uint64_t version_;
    HALHazardNodeI *node_;
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
  T *remove_from_slist(T *node, T *&head);

  template <uint16_t NestSize, int64_t NodeCap>
  class ThreadStoreT {
    typedef ThreadStoreT<NestSize, NodeCap> TS;
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
      int64_t get_node_count() const;
      void retire(const uint64_t version);

      uint64_t get_min_version();
    private:
      bool enabled_;
      pthread_spinlock_t lock_;

      VersionNode version_nodes_[NestSize];
      VersionNode *version_free_list_;
      VersionNode *version_using_list_;

      HazardNode hazard_nodes_[NodeCap];
      HazardNode *hazard_free_list_;
      HazardNode *hazard_waiting_list_;
      int64_t hazard_waiting_count_;

      TS *next_;
  };
}

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  class HALHazardNodeI {
    public:
      virtual ~HALHazardNodeI() {}
      virtual void retire() = 0;
  };

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread, int64_t MaxWaitingCntPerThread>
  class HALHazardVersionT {
    typedef hazard_version::ThreadStoreT<MaxNestCntPerThread, MaxWaitingCntPerThread> TS;
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
      uint64_t get_min_version_(const bool flush);
    private:
      pthread_spinlock_t lock_;
      uint64_t version_;
      TS threads_[MaxThreadCnt];
      TS *thread_list_;

      uint64_t curr_min_version_;
      int64_t curr_min_version_timestamp_;
  };

  typedef HALHazardVersionT<HAL_MAX_THREAD_COUNT, 16, 64> HALHazardVersion;

  ////////////////////////////////////////////////////////////////////////////////////////////////////

namespace hazard_version {

  template <typename T>
  T *remove_from_slist(T *node, T *&head) {
    T *node2remove = NULL;
    if (NULL != node->next_) {
      node2remove = node->next_;
      T tmp = *node;
      *node = *node->next_;
      *node2remove = tmp;
    } else {
      node2remove = node;
      head = NULL;
    }
    return node2remove;
  }

  template <uint16_t NestSize, int64_t NodeCap>
  ThreadStoreT<NestSize, NodeCap>::ThreadStoreT() 
    : enabled_(false),
      version_free_list_(NULL),
      version_using_list_(NULL),
      hazard_free_list_(NULL),
      hazard_waiting_list_(NULL),
      hazard_waiting_count_(0),
      next_(NULL) {
    pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
    memset(version_nodes_, 0, sizeof(version_nodes_));
    memset(hazard_nodes_, 0, sizeof(hazard_nodes_));
    for (uint16_t i = 0; i < NestSize; i++) {
      version_nodes_[i].next_ = version_free_list_;
      version_nodes_[i].pos_ = i;
      version_free_list_ = &version_nodes_[i];
    }
    for (int64_t i = 0; i < NodeCap; i++) {
      hazard_nodes_[i].next_ = hazard_free_list_;
      hazard_free_list_ = &hazard_nodes_[i];
    }
  }

  template <uint16_t NestSize, int64_t NodeCap>
  ThreadStoreT<NestSize, NodeCap>::~ThreadStoreT() {
    while (NULL != hazard_waiting_list_) {
      hazard_waiting_list_->node_->retire();
      hazard_waiting_list_ = hazard_waiting_list_->next_;
    }
    pthread_spin_destroy(&lock_);
  }

  template <uint16_t NestSize, int64_t NodeCap>
  void ThreadStoreT<NestSize, NodeCap>::set_enabled() {
    enabled_ = true;
  }

  template <uint16_t NestSize, int64_t NodeCap>
  bool ThreadStoreT<NestSize, NodeCap>::is_enabled() const {
    return enabled_;
  }

  template <uint16_t NestSize, int64_t NodeCap>
  void ThreadStoreT<NestSize, NodeCap>::set_next(TS *ts) {
    next_ = ts;
  }

  template <uint16_t NestSize, int64_t NodeCap>
  ThreadStoreT<NestSize, NodeCap> *ThreadStoreT<NestSize, NodeCap>::get_next() const {
    return next_;
  }

  template <uint16_t NestSize, int64_t NodeCap>
  int ThreadStoreT<NestSize, NodeCap>::acquire(const uint64_t version, VersionHandle &handle) {
    int ret = HAL_SUCCESS;
    pthread_spin_lock(&lock_);
    if (NULL == version_free_list_) {
      LOG_WARN(CLIB, "no version node, maybe nest too much");
      ret = HAL_OUT_OF_RESOURCES;
    } else {
      VersionNode *version_node = version_free_list_;
      version_free_list_ = version_free_list_->next_;

      version_node->version_ = version;

      version_node->next_ = version_using_list_;
      version_using_list_ = version_node;

      handle.pos_ = version_node->pos_;
      handle.seq_ = version_node->seq_;
    }
    pthread_spin_unlock(&lock_);
    return ret;
  }

  template <uint16_t NestSize, int64_t NodeCap>
  void ThreadStoreT<NestSize, NodeCap>::release(const VersionHandle &handle) {
    pthread_spin_lock(&lock_);
    if (NestSize <= handle.pos_) {
      LOG_WARN(CLIB, "invalid handle, pos=%hu", handle.pos_);
    } else if (version_nodes_[handle.pos_].seq_ != handle.seq_) {
      LOG_WARN(CLIB, "invalid handle, seq=%u", handle.seq_);
    } else {
      __sync_add_and_fetch(&version_nodes_[handle.pos_].seq_, 1);
      VersionNode *node2free = remove_from_slist(&version_nodes_[handle.pos_], version_using_list_);
      node2free->next_ = version_free_list_;
      version_free_list_ = node2free;
    }
    pthread_spin_unlock(&lock_);
  }

  template <uint16_t NestSize, int64_t NodeCap>
  int ThreadStoreT<NestSize, NodeCap>::add_node(const uint64_t version, HALHazardNodeI *node) {
    int ret = HAL_SUCCESS;
    pthread_spin_lock(&lock_);
    if (NULL == hazard_free_list_) {
      LOG_WARN(CLIB, "no hazard node available");
      ret = HAL_OUT_OF_RESOURCES;
    } else {
      HazardNode *hazard_node = hazard_free_list_;
      hazard_free_list_ = hazard_free_list_->next_;

      hazard_node->version_ = version;
      hazard_node->node_ = node;

      hazard_node->next_ = hazard_waiting_list_;
      hazard_waiting_list_ = hazard_node;
      
      hazard_waiting_count_++;
    }
    pthread_spin_unlock(&lock_);
    return ret;
  }

  template <uint16_t NestSize, int64_t NodeCap>
  int64_t ThreadStoreT<NestSize, NodeCap>::get_node_count() const {
    return hazard_waiting_count_;
  }

  template <uint16_t NestSize, int64_t NodeCap>
  void ThreadStoreT<NestSize, NodeCap>::retire(const uint64_t version) {
    HazardNode *list2retire = NULL;

    pthread_spin_lock(&lock_);
    HazardNode pseudo_head;
    pseudo_head.next_ = hazard_waiting_list_;
    HazardNode *iter = &pseudo_head;
    while (NULL != iter->next_) {
      if (iter->next_->version_ < version) {
        HazardNode *tmp = iter->next_;
        iter->next_ = iter->next_->next_;

        tmp->next_ = list2retire;
        list2retire = tmp;
      } else {
        iter = iter->next_;
      }
    }
    hazard_waiting_list_ = pseudo_head.next_;
    pthread_spin_unlock(&lock_);

    while (NULL != list2retire) {
      HazardNode *node2retire = list2retire;
      list2retire = list2retire->next_;

      node2retire->node_->retire();
      node2retire->next_ = hazard_free_list_;
      hazard_free_list_ = node2retire;
    }
  }

  template <uint16_t NestSize, int64_t NodeCap>
  uint64_t ThreadStoreT<NestSize, NodeCap>::get_min_version() {
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

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread, int64_t MaxWaitingCntPerThread>
  HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread, MaxWaitingCntPerThread>::HALHazardVersionT()
    : version_(0), 
      thread_list_(NULL),
      curr_min_version_(0),
      curr_min_version_timestamp_(0) {
    pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread, int64_t MaxWaitingCntPerThread>
  HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread, MaxWaitingCntPerThread>::~HALHazardVersionT() {
    pthread_spin_destroy(&lock_);
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread, int64_t MaxWaitingCntPerThread>
  int HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread, MaxWaitingCntPerThread>::add_node(HALHazardNodeI *node) {
    int ret = HAL_SUCCESS;
    TS *ts = NULL;
    if (NULL == node) {
      LOG_WARN(CLIB, "invalid param, node null pointer");
      ret = HAL_INVALID_PARAM;
    } else if (HAL_SUCCESS != (ret = get_thread_store_(ts, NULL))) {
      LOG_WARN(CLIB, "get_thread_store_ fail, ret=%d", ret);
    } else {
      bool flush_min_version = false;
      for (int64_t i = 0; i < 3; i++) {
        if (HAL_OUT_OF_RESOURCES == ret) {
          ts->retire(get_min_version_(flush_min_version));
          flush_min_version = true;
        }
        if (HAL_SUCCESS != (ret = ts->add_node(__sync_fetch_and_add(&version_, 1), node))
            && HAL_OUT_OF_RESOURCES == ret) {
          continue;
        } else {
          break;
        }
      }
    }
    return ret;
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread, int64_t MaxWaitingCntPerThread>
  int HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread, MaxWaitingCntPerThread>::acquire(uint64_t &handle) {
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

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread, int64_t MaxWaitingCntPerThread>
  void HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread, MaxWaitingCntPerThread>::release(const uint64_t handle) {
    hazard_version::VersionHandle version_handle(handle);
    if (MaxThreadCnt > version_handle.tid_) {
      TS *ts = &threads_[version_handle.tid_];
      ts->release(version_handle);
      get_min_version_(false);
    }
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread, int64_t MaxWaitingCntPerThread>
  int HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread, MaxWaitingCntPerThread>::get_thread_store_(TS *&ts, uint16_t *tid) {
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
        }
        pthread_spin_unlock(&lock_);
      }
    }
    return ret;
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread, int64_t MaxWaitingCntPerThread>
  uint64_t HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread, MaxWaitingCntPerThread>::get_min_version_(const bool flush) {
    uint64_t ret = 0;
    if (!flush
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
    LOG_DEBUG(CLIB, "get_min_version_=%lu", ret);
    return ret;
  }

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCntPerThread, int64_t MaxWaitingCntPerThread>
  void HALHazardVersionT<MaxThreadCnt, MaxNestCntPerThread, MaxWaitingCntPerThread>::retire() {
    uint64_t min_version = get_min_version_(true);
    TS *iter = ATOMIC_LOAD(&thread_list_);
    while (NULL != iter) {
      iter->retire(min_version);
      iter = iter->get_next();
    }
  }
}
}

#endif // __HAL_CLIB_HAZARD_VERSION_H__
