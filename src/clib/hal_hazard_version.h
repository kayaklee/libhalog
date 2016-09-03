// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_HAZARD_VERSION_H__
#define __HAL_CLIB_HAZARD_VERSION_H__
#include <stdint.h>

#include "hal_error.h"

namespace libhalog {
namespace clib {
namespace hazard_version {
  struct VersionNode {
    VersionNode *next_;
    uint64_t version_;
    uint32_t seq_;
  };

  struct Handle {
    union {
      struct {
        uint16_t tid_;
        uint16_t pos_;
        uint32_t seq_;
      };
      uint64_t u64_;
    };
  };

  class IHALHazardNode;
  template <uint16_t size>
  class ThreadStore {
    typedef ThreadStore<size> TS;
    public:
      ThreadVersionStore();
    public:
      void set_next(TS *ts);
      TS *get_next() const;

      int acquire(const uint64_t version, Handle &handle);
      void release(const Handle &handle);

      int add_node(IHALHazardNode *node);
      int64_t get_node_count() const;
      void retire(const uint64_t version);
    private:
      pthread_spinlock_t lock_;

      VersionNode nodes_[size];
      VersionNode *using_list_;
      VersionNode *free_list_;

      IHALHazardNode *retire_waiting_list_;
      int64_t retire_waiting_count_;

      ThreadVersionStore *next_;
  };
}
  class IHALHazardNode {
    public:
      virtual ~IHALHazardNode() {}
      virtual void retire() = 0;
    public:
      void set_version(const uint64_t version);
      uint64_t get_version() const;
      void set_next(IHALHazardNode *node);
      IHALHazardNode *get_next() const;
    private:
      uint64_t version_;
      IHALHazardNode *next_;
  };

  template <uint16_t MaxThreadCnt, uint16_t MaxNestCnt>
  class HALHazardVersion {
    typedef hazard_version::ThreadStore<MaxNestCnt> TS;
    public:
      HALHazardVersion();
      ~HALHazardVersion();
    public:
      int add_node(IHALHazardNode *node);
      int acquire(uint64_t &handle);
      int release(const uint64_t handle);
    private:
      int get_thread_store_(TS *&ts);
    private:
      volatile uint64_t version_;
      TS threads_[MaxThreadCnt];
      volatile TS *thread_list_;
  };
}
}

#endif // __HAL_CLIB_HAZARD_VERSION_H__
