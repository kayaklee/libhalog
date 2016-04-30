// Libhalog
// Author: likai.root@gmail.com

#include "hal_error.h"

namespace libhalog {
namespace clib {

  const char *hal_strerror(const int hal_error) {
    static HALErrorString error_string;
    return error_string.strerror(hal_error);
  }

}
}

