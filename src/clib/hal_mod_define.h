// Libhalog
// Author: likai.root@gmail.com

#ifdef HAL_MOD_DEF
HAL_MOD_DEF(CLIB)
HAL_MOD_DEF(FIXED_QUEUE)
HAL_MOD_DEF(END)
#endif

#ifndef __HAL_CLIB_MOD_DEFINE_H__
#define __HAL_CLIB_MOD_DEFINE_H__
#include <stdint.h>
#include "hal_define.h"

namespace libhalog {
namespace clib {

  class HALModIds {
    public:
      enum {
#define HAL_MOD_DEF(name) name,
#include "clib/hal_mod_define.h"
#undef HAL_MOD_DEF
      };
  };

  class HALModItem {
    static const int64_t MAX_STR_LENGTH = 1024;
    public:
      HALModItem();
      ~HALModItem();
    public:
      void set_mod(const int mod_id, const char *mod_str);
      void on_alloc(const int64_t size);
      void on_free(const int64_t size);
      const char *cstring();
    private:
      int mod_id_;
      const char *mod_str_;
      int64_t alloc_size_[HAL_MAX_THREAD_COUNT];
      int64_t free_size_[HAL_MAX_THREAD_COUNT];
      int64_t alloc_count_[HAL_MAX_THREAD_COUNT];
      int64_t free_count_[HAL_MAX_THREAD_COUNT];
  };

  class HALModSet {
    public:
      HALModSet() {
#define HAL_MOD_DEF(name) set_mod_item_(HALModIds::name, #name);
#include "clib/hal_mod_define.h"
#undef HAL_MOD_DEF
      };
      ~HALModSet() {}
    public:
      inline void on_alloc(const int mod_id, const int64_t size) {
        if (mod_id >= 0 && mod_id < HALModIds::END) {
          mods_[mod_id].on_alloc(size);
        }
      }
      inline void on_free(const int mod_id, const int64_t size) {
        if (mod_id >= 0 && mod_id < HALModIds::END) {
          mods_[mod_id].on_free(size);
        }
      }
      void print_usage();
    private:
      inline void set_mod_item_(const int mod_id, const char *mod_str) {
        if (mod_id >= 0 && mod_id < HALModIds::END) {
          mods_[mod_id].set_mod(mod_id, mod_str);
        }
      }
    public:
      HALModItem mods_[HALModIds::END];
  };

}
}

#endif // __HAL_CLIB_MOD_DEFINE_H__
