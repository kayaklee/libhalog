// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_MALLOC_H__
#define __HAL_CLIB_MALLOC_H__
#include "hal_define.h"

namespace libhalog {
namespace clib {

  extern void *hal_malloc(const int64_t size, const int mod_id);

  extern void hal_free(void *ptr);

  extern void hal_malloc_print_usage();

}
}

#endif // __HAL_CLIB_MALLOC_H__
