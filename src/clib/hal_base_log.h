// Libhalog
// Author: likai.root@gmail.com

#ifdef HAL_LOG_LEVEL_DEF
HAL_LOG_LEVEL_DEF(DEBUG)
HAL_LOG_LEVEL_DEF(TRACE)
HAL_LOG_LEVEL_DEF(INFO)
HAL_LOG_LEVEL_DEF(WARN)
HAL_LOG_LEVEL_DEF(ERROR)
HAL_LOG_LEVEL_DEF(END)
#endif

#ifndef __HAL_CLIB_BASE_LOG_H__
#define __HAL_CLIB_BASE_LOG_H__
#include <stdint.h>
#include <pthread.h>

namespace libhalog {
namespace clib {

  class HALLogLevels {
    public:
      enum {
#define HAL_LOG_LEVEL_DEF(name) HAL_LOG_##name,
#include "clib/hal_base_log.h"
#undef HAL_LOG_LEVEL_DEF
      };
  };

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  class IHALLogLevelFilter {
    public:
      virtual ~IHALLogLevelFilter() {}
    public:
      virtual bool i_if_output(const int32_t level) const = 0;
  };

  class IHALLogLevelString {
    public:
      virtual ~IHALLogLevelString() {}
    public:
      virtual const char *i_level_string(const int64_t level) const = 0;
  };

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  class HALLogLevelFilterDefault: public IHALLogLevelFilter {
    public:
      bool i_if_output(const int32_t level) const {
        return true;
      }
  };

  class HALLogLevelStringDefault : public IHALLogLevelString {
    static const int64_t DEFAULT_LOG_LEVEL_COUNT = HALLogLevels::HAL_LOG_END;
    public:
      HALLogLevelStringDefault() {
#define HAL_LOG_LEVEL_DEF(name) set_level_string_(HALLogLevels::HAL_LOG_##name, #name);
#include "clib/hal_base_log.h"
#undef HAL_LOG_LEVEL_DEF
      }
    public:
      const char *i_level_string(const int64_t level) const {
        const char *ret = "UNKNOW";
        if (level >= 0 && level < DEFAULT_LOG_LEVEL_COUNT) {
          ret = level_strings_[level];
        }
        return ret;
      }
    private:
      inline void set_level_string_(const int64_t level, const char *string) {
        if (level >= 0 && level < DEFAULT_LOG_LEVEL_COUNT) {
          level_strings_[level] = string;
        }
      }
    private:
      const char *level_strings_[DEFAULT_LOG_LEVEL_COUNT];
  };

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  
  class HALLog {
    static const mode_t LOG_FILE_MODE = 0644;
    public:
      HALLog();
      virtual ~HALLog();
    public:
      int set_file_name(const char *file_name, const bool redirect_std);

      int set_max_size(const int64_t size);

      int set_switch_time(const int64_t hour, const int64_t minute);

      int set_level_filter(const IHALLogLevelFilter *level_filter);

      int set_level_string(const IHALLogLevelString *level_string);

      int set_redirect_std();

      int set_check_file_exist(const bool check_file_exist);
    public:
      int write_log(
          const char *module,
          const int32_t level,
          const char *file,
          const int32_t line,
          const char *function,
          const char *fmt,
          ...) __attribute__((format(printf, 7, 8)));

      template <typename... Args>
      int write_kv(
          const char *module,
          const int32_t level,
          const char *file,
          const int32_t line,
          const char *function,
          const Args&... args);
    public:
      int get_currnet_fd_();
    private:
      pthread_rwlock_t file_name_lock_;
      const char *file_name_;
      int fd_;
      int64_t pos_;

      int64_t max_size_;
      int64_t switch_hour_;
      int64_t switch_minute_;

      HALLogLevelFilterDefault level_filter_default_;
      const IHALLogLevelFilter *level_filter_;
      HALLogLevelStringDefault level_string_default_;
      const IHALLogLevelString *level_string_;

      bool redirect_std_;
      bool check_file_exist_;
  };

}
}

#endif // __HAL_CLIB_BASE_LOG_H__
