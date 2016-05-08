// Libhalog
// Author: likai.root@gmail.com

#include "hal_base_log.h"
#include "hal_util.h"

namespace libhalog {
namespace clib {

  void lkv(const char *name, void *ptr) {
    LOG_INFO(CLIB, "new gsi [%s] [%p]", name, ptr);
  }

}
}

