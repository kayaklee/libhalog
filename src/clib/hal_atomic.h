// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_ATOMIC_H__
#define __HAL_CLIB_ATOMIC_H__

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#define __COMPILER_BARRIER() asm volatile("" ::: "memory")
#define MEM_BARRIER() __sync_synchronize()
#define PAUSE() asm("pause\n")

#if GCC_VERSION > 40704
#define ATOMIC_LOAD(x) __atomic_load_n((x), __ATOMIC_SEQ_CST)
#define ATOMIC_STORE(x, v) __atomic_store_n((x), (v), __ATOMIC_SEQ_CST)
#else
#define ATOMIC_LOAD(x) ({__COMPILER_BARRIER(); *(x);})
#define ATOMIC_STORE(x, v) ({__COMPILER_BARRIER(); *(x) = v; __sync_synchronize(); })
#endif

#define CACHE_ALIGN_SIZE 64
#define CACHE_ALIGNED __attribute__((aligned(CACHE_ALIGN_SIZE)))

namespace libhalog {
namespace clib {
}
}

#endif // __HAL_CLIB_ATOMIC_H__
