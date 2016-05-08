// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_PAGE_ARENA_H__
#define __HAL_CLIB_PAGE_ARENA_H__
#include <stdint.h>
#include "hal_util.h"
#include "hal_base_log.h"
#include "hal_i_allocator.h"

namespace libhalog {
namespace clib {
namespace pagearena {
    struct NormalPage {
      NormalPage *next;
      int32_t pos;
      char buf[0];
    };

    struct BigPage {
      BigPage *next;
      char buf[0];
    };
}
  class HALPageArena {
    public:
      HALPageArena(const int64_t page_size, IHALAllocator &allocator, const int mod_id);
      ~HALPageArena();
    public:
      void *alloc(const int64_t size);
      void reuse();
      void reset();
      int64_t used() const;
      int64_t total() const;
      int64_t count() const;
    private:
      pagearena::NormalPage *get_normal_page_(const int64_t alloc_size);
      pagearena::BigPage *get_big_page_(const int64_t page_size);
    private:
      const int64_t page_size_;
      IHALAllocator &allocator_;
      const int mod_id_;
      pagearena::NormalPage *free_page_list_;
      pagearena::NormalPage *normal_page_list_;
      pagearena::BigPage *big_page_list_;
      int64_t used_;
      int64_t total_;
      int64_t count_;
  };

  template <int64_t PAGE_SIZE, class Allocator>
  class HALPageArenaT : public IHALAllocator, public HALPageArena {
    public:
      HALPageArenaT(const int mod_id, Allocator &allocator);
    public:
      void *alloc(const int64_t size);
      void *alloc(const int64_t size, const int mod_id);
      void free(void *ptr);
  };

  template <int64_t PAGE_SIZE, class Allocator>
  HALPageArenaT<PAGE_SIZE, Allocator>::HALPageArenaT(const int mod_id, Allocator &allocator)
    : HALPageArena(PAGE_SIZE, allocator, mod_id) {
  }

  template <int64_t PAGE_SIZE, class Allocator>
  void *HALPageArenaT<PAGE_SIZE, Allocator>::alloc(const int64_t size) {
    return HALPageArena::alloc(size);
  }

  template <int64_t PAGE_SIZE, class Allocator>
  void *HALPageArenaT<PAGE_SIZE, Allocator>::alloc(const int64_t size, const int mod_id) {
    UNUSED(mod_id);
    return HALPageArena::alloc(size);
  }

  template <int64_t PAGE_SIZE, class Allocator>
  void HALPageArenaT<PAGE_SIZE, Allocator>::free(void *ptr) {
    UNUSED(ptr);
    LOG_WARN(CLIB, "Not implement.");
  }

  typedef HALPageArenaT<HAL_NORMAL_ALLOC_PAGE, HALDefaultAllocator> HAL64KPageArena;
  typedef HALPageArenaT<HAL_BIG_ALLOC_PAGE, HALDefaultAllocator> HAL2MPageArena;
}
}

#endif // __HAL_CLIB_PAGE_ARENA_H__
