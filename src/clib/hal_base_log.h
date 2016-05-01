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
#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include "clib/hal_spin_rwlock.h"

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
        UNUSED(level);
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
    static const int64_t DEFAULT_MAX_LOG_FILE_SIZE = 1L*1024L*1024L*1024L;
    static const int64_t MAX_FILE_NAME_LENGTH = 4096;
    static const int64_t MAX_LOG_HEADER_SIZE = 256;
    static const int64_t MAX_LOG_CONTENT_SIZE = 4096;
    static const mode_t LOG_FILE_MODE = 0644;
    static const mode_t LOG_DIR_MODE = 0775;
    public:
      HALLog();
      virtual ~HALLog();
    public:
      int open_log(const char *file_name, const bool redirect_std, const bool switch_file);

      int set_max_size(const int64_t size);

      int set_switch_time(const int64_t hour, const int64_t minute);

      int set_level_filter(const IHALLogLevelFilter *level_filter);

      int set_level_string(const IHALLogLevelString *level_string);

      int set_check_file_exist(const bool check_file_exist);
    public:
      void write_log(
          const char *module,
          const int32_t level,
          const char *file,
          const int32_t line,
          const char *function,
          const char *fmt,
          ...) __attribute__((format(printf, 7, 8)));

      template <typename... Args>
      void write_kv(
          const char *module,
          const int32_t level,
          const char *file,
          const int32_t line,
          const char *function,
          const Args&... args);
    private:
      void create_log_dir_(const char *file_name);
      bool need_switch_file_(const int64_t reserve_size);
      void switch_file_(const int64_t reserve_size, const bool force);
      int get_fd_(const int64_t reserve_size, HALRLockGuard &lock_guard);
      const char *format_log_header_(
          const char *module,
          const int32_t level,
          const char *file,
          const int32_t line,
          const char *function,
          int64_t &header_length);
    private:
      HALSpinRWLock file_lock_;
      const char *file_name_;
      int fd_;
      struct tm fd_tm_;
      bool redirect_std_;

      int64_t pos_;
      int64_t max_size_;
      int64_t switch_hour_;
      int64_t switch_minute_;
      bool check_file_exist_;

      HALLogLevelFilterDefault level_filter_default_;
      const IHALLogLevelFilter *level_filter_;
      HALLogLevelStringDefault level_string_default_;
      const IHALLogLevelString *level_string_;
  };

}
}

#endif // __HAL_CLIB_BASE_LOG_H__
