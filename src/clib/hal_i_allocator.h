// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_I_ALLOCATOR_H__
#define __HAL_CLIB_I_ALLOCATOR_H__
#include "hal_define.h"
#include "hal_malloc.h"

namespace libhalog {
namespace clib {

  class IHALAllocator {
    public:
      virtual ~IHALAllocator() {}
    public:
      virtual void *alloc(const int64_t size, const int mod_id) = 0;
      virtual void free(void *ptr) = 0;
  };

  class HALDefaultAllocator : public IHALAllocator {
    public:
      void *alloc(const int64_t size, const int mod_id) {
        return hal_malloc(size, mod_id);
      }
      void free(void *ptr) {
        hal_free(ptr);
      }
  };

}
}

#endif // __HAL_CLIB_I_ALLOCATOR_H__
