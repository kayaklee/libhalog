// Libhalog
// Author: likai.root@gmail.com

#include <stdlib.h>
#include "hal_util.h"
#include "hal_malloc.h"
#include "hal_mod_define.h"

namespace libhalog {
namespace clib {

  struct HALMemoryHeader {
    int mod_id;
    int32_t size;
    char buf[0];
  };

  void *hal_malloc(const int64_t size, const int mod_id) {
    void *ret = NULL;
    int64_t alloc_size = size + sizeof(HALMemoryHeader);
    HALMemoryHeader *memory_header = (HALMemoryHeader*)malloc(alloc_size);
    if (NULL != memory_header) {
      memory_header->mod_id = mod_id;
      memory_header->size = (int32_t)size;
      gsi<HALModSet>().on_alloc(mod_id, size);
      ret = memory_header->buf;
    }
    return ret;
  }

  void hal_free(void *ptr) {
    if (NULL != ptr) {
      HALMemoryHeader *memory_header = (HALMemoryHeader*)((char*)ptr - (char*)&(((HALMemoryHeader*)0)->buf));
      gsi<HALModSet>().on_free(memory_header->mod_id, memory_header->size);
      free(memory_header);
    }
  }

  void hal_malloc_print_usage() {
    gsi<HALModSet>().print_usage();
  }

}
}

