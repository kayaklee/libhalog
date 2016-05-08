// Libhalog
// Author: likai.root@gmail.com

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "hal_base_log.h"
#include "hal_error.h"
#include "hal_util.h"

namespace libhalog {
namespace clib {

  HALLog::HALLog()
    : file_lock_(),
      file_name_(NULL),
      fd_(-1),
      redirect_std_(false),
      pos_(0),
      max_size_(DEFAULT_MAX_LOG_FILE_SIZE),
      switch_hour_(-1),
      switch_minute_(-1),
      check_file_exist_(false),
      level_filter_(&level_filter_default_),
      level_string_(&level_string_default_) {
  }

  HALLog::~HALLog() {
    if (-1 != fd_) {
      close(fd_);
      fd_ = -1;
    }
    if (NULL != file_name_) {
      free((void*)file_name_);
      file_name_ = NULL;
    }
  }

  int HALLog::open_log(const char *file_name, const bool redirect_std, const bool switch_file) {
    int ret = HAL_SUCCESS;
    const char *new_file_name = NULL;
    int new_fd = -1;
    if (-1 != fd_) {
      ret = HAL_INIT_REPETITIVE;
    } else if (NULL == file_name) {
      ret = HAL_INVALID_PARAM;
    } else if (NULL == (new_file_name = strdup(file_name))) {
      ret = HAL_ALLOCATE_FAIL;
    } else {
      create_log_dir_(new_file_name);
      if (-1 == (new_fd = open(new_file_name, O_RDWR | O_CREAT | O_APPEND, LOG_FILE_MODE))) {
        fprintf(stderr, "open log file [%s] fail, err=[%s]\n", new_file_name, strerror(errno));
        ret = HAL_OPEN_FILE_FAIL;
      } else {
        file_name_ = new_file_name;
        redirect_std_ = redirect_std;
        fd_ = new_fd;
        get_cur_tm(fd_tm_);
        if (redirect_std) {
          dup2(fd_, STDOUT_FILENO);
          dup2(fd_, STDERR_FILENO);
        }
        if (0 > (pos_ = lseek(fd_, 0, SEEK_END))) {
          pos_ = 0;
        }
        if (switch_file
            && 0 < pos_) {
          bool force = true;
          switch_file_(0, force);
        }
      }
    }
    if (HAL_SUCCESS != ret) {
      if (NULL != new_file_name) {
        free((void*)new_file_name);
        new_file_name = NULL;
      }
      if (-1 != new_fd) {
        close(new_fd);
        new_fd = -1;
      }
    }
    return ret;
  }

  int HALLog::set_max_size(const int64_t size) {
    int ret = HAL_SUCCESS;
    if (size <= 0) {
      ret = HAL_INVALID_PARAM;
    } else {
      max_size_ = size;
    }
    return ret;
  }

  int HALLog::set_switch_time(const int64_t hour, const int64_t minute) {
    int ret = HAL_SUCCESS;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
      ret = HAL_INVALID_PARAM;
    } else {
      switch_hour_ = hour;
      switch_minute_ = minute;
    }
    return ret;
  }

  int HALLog::set_level_filter(const IHALLogLevelFilter *level_filter) {
    int ret = HAL_SUCCESS;
    if (NULL == level_filter) {
      level_filter_ = &level_filter_default_;
    } else {
      level_filter_ = level_filter;
    }
    return ret;
  }

  int HALLog::set_level_string(const IHALLogLevelString *level_string) {
    int ret = HAL_SUCCESS;
    if (NULL == level_string) {
      level_string_ = &level_string_default_;
    } else {
      level_string_ = level_string;
    }
    return ret;
  }

  int HALLog::set_check_file_exist(const bool check_file_exist) {
    int ret = HAL_SUCCESS;
    check_file_exist_ = check_file_exist;
    return ret;
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  
  void HALLog::create_log_dir_(const char *file_name) {
    const char *base_file_name = strrchr(file_name, '/');
    if (NULL == base_file_name) {
      return;
    } else {
      int64_t dir_length = base_file_name - file_name;
      char dir[MAX_FILE_NAME_LENGTH];
      snprintf(dir, sizeof(dir), "%.*s", (int)dir_length, file_name);
      create_log_dir_(dir);
      mkdir(dir, LOG_DIR_MODE);
    }
  }

  bool HALLog::need_switch_file_(const int64_t reserve_size) {
    bool bret = false;
    if ((pos_ + reserve_size) > max_size_) {
      bret = true;
    }
    if (!bret
        && -1 != switch_hour_) {
      const struct tm *cur_tm = get_cur_tm();
      if ((fd_tm_.tm_mday != cur_tm->tm_mday
            || fd_tm_.tm_mon != cur_tm->tm_mon
            || fd_tm_.tm_year != cur_tm->tm_year)
          && cur_tm->tm_hour >= switch_hour_
          && cur_tm->tm_min >= switch_minute_) {
        bret = true;
      }
    }
    if (!bret
        && check_file_exist_) {
      if (NULL != file_name_
          && 0 != access(file_name_, F_OK)) {
        bret = true;
      }
    }
    return bret;
  }

  int HALLog::get_fd_(const int64_t reserve_size, HALRLockGuard &lock_guard) {
    int ret = -1;
    if (-1 == fd_) {
      ret = STDERR_FILENO;
    } else {
      if (need_switch_file_(reserve_size)) {
        bool force = false;
        switch_file_(reserve_size, force);
      }
      file_lock_.rlock();
      lock_guard.set_lock(&file_lock_);
      ret = fd_;
    }
    return ret;
  }

  void HALLog::switch_file_(const int64_t reserve_size, const bool force) {
    if (NULL != file_name_) {
      file_lock_.lock();
      if (force
          || need_switch_file_(reserve_size)) {
        const struct tm *cur_tm = get_cur_tm();
        int64_t usec = get_cur_microseconds_time() % 1000000;
        char new_file_name[MAX_FILE_NAME_LENGTH];
        snprintf(new_file_name, sizeof(new_file_name), "%s.%04d%02d%02d%02d%02d%02d.%ld",
            file_name_,
            cur_tm->tm_year + 1900,
            cur_tm->tm_mon + 1,
            cur_tm->tm_mday,
            cur_tm->tm_hour,
            cur_tm->tm_min,
            cur_tm->tm_sec,
            usec);
        rename(file_name_, new_file_name);

        int new_fd = -1;
        if (-1 != (new_fd = open(file_name_, O_RDWR | O_CREAT | O_APPEND, LOG_FILE_MODE))) {
          close(fd_);
          fd_ = new_fd;
          get_cur_tm(fd_tm_);
          if (redirect_std_) {
            dup2(fd_, STDOUT_FILENO);
            dup2(fd_, STDERR_FILENO);
          }
          if (0 > (pos_ = lseek(fd_, 0, SEEK_END))) {
            pos_ = 0;
          }
        }
      }
      file_lock_.unlock();
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  
  const char *HALLog::format_log_header_(
      const char *module,
      const int32_t level,
      const char *file,
      const int32_t line,
      const char *function,
      int64_t &header_length) {
    static __thread char header[MAX_LOG_HEADER_SIZE];
    const struct tm *cur_tm = get_cur_tm();
    int64_t usec = get_cur_microseconds_time() % 1000000;
    const char *base_file_name = strrchr(file, '/');
    base_file_name = base_file_name ? base_file_name + 1 : file;
    header_length = snprintf(header, MAX_LOG_HEADER_SIZE, "[%04d-%02d-%02d %02d:%02d:%02d.%06ld] "
        "%s %s %s:%d:%s [%ld] ",
        cur_tm->tm_year + 1900,
        cur_tm->tm_mon + 1,
        cur_tm->tm_mday,
        cur_tm->tm_hour,
        cur_tm->tm_min,
        cur_tm->tm_sec,
        usec,
        level_string_->i_level_string(level),
        module,
        base_file_name,
        line,
        function,
        gettid());
    if (header_length >= MAX_LOG_HEADER_SIZE) {
      header_length = MAX_LOG_HEADER_SIZE - 1;
    } else if (header_length < 0) {
      header_length = 0;
    }
    return header;
  }

  char NEWLINE[1] = {'\n'};

  void HALLog::write_log(
      const char *module,
      const int32_t level,
      const char *file,
      const int32_t line,
      const char *function,
      const char *fmt,
      ...) {
    if (!level_filter_->i_if_output(level)) {
      return;
    }

    static __thread char content[MAX_LOG_CONTENT_SIZE];
    va_list args;
    va_start(args, fmt);
    int64_t content_length = vsnprintf(content, MAX_LOG_CONTENT_SIZE, fmt, args);
    va_end(args);
    if (content_length >= MAX_LOG_CONTENT_SIZE) {
      content_length = MAX_LOG_CONTENT_SIZE - 1;
    }
    if (0 >= content_length) {
      return;
    }

    int64_t header_length = 0;
    const char *header = format_log_header_(module, level, file, line, function, header_length);

    struct iovec vec[3];
    vec[0].iov_base = (void*)header;
    vec[0].iov_len = header_length;
    vec[1].iov_base = content;
    vec[1].iov_len = content_length;
    vec[2].iov_base = NEWLINE;
    vec[2].iov_len = sizeof(NEWLINE);

    int64_t log_size = header_length + content_length + sizeof(NEWLINE);
    HALRLockGuard lock_guard;
    int fd = get_fd_(log_size, lock_guard);
    int64_t write_length = writev(fd, vec, ARRAYSIZE(vec));
    assert(write_length == log_size);
    __sync_add_and_fetch(&pos_, log_size);
  }

}
}

