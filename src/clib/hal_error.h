// Libhalog
// Author: likai.root@gmail.com

#ifdef HAL_ERROR_DEF
HAL_ERROR_DEF(HAL_INVALID_PARAM) //-10000
HAL_ERROR_DEF(HAL_ALLOCATE_FAIL)
HAL_ERROR_DEF(HAL_INIT_REPETITIVE)
HAL_ERROR_DEF(HAL_OPEN_FILE_FAIL)
#endif

#ifndef __HAL_CLIB_ERROR_H__
#define __HAL_CLIB_ERROR_H__
#include <stdint.h>
#include <pthread.h>

namespace libhalog {
namespace clib {

  enum {
    __HAL_ERRNO_START__ = -10001,
#define HAL_ERROR_DEF(error) error,
#include "clib/hal_error.h"
#undef HAL_ERROR_DEF
    HAL_ERROR = -1,
    HAL_SUCCESS = 0,
  };

  extern const char *hal_strerror(const int hal_error);

  class HALErrorString {
    static const int64_t MAX_ERROR_COUNT = -__HAL_ERRNO_START__;
    public:
      HALErrorString() {
#define HAL_ERROR_DEF(error) set_error_string_(error, #error);
#include "clib/hal_error.h"
#undef HAL_ERROR_DEF
        set_error_string_(HAL_ERROR, "HAL_ERROR");
        set_error_string_(HAL_SUCCESS, "HAL_SUCCESS");
      }
    public:
      inline const char *strerror(const int hal_error) {
        const char *str = "UNKNOW";
        int index = -hal_error;
        if (index >= 0 && index < MAX_ERROR_COUNT) {
          str = error_strings_[index];
        }
        return str;
      }
    private:
      inline void set_error_string_(const int hal_error, const char *string) {
        int index = -hal_error;
        if (index >= 0 && index < MAX_ERROR_COUNT) {
          error_strings_[index] = string;
        }
      }
    private:
      const char *error_strings_[MAX_ERROR_COUNT];
  };

}
}

#endif // __HAL_CLIB_ERROR_H__
