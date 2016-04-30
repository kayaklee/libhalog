// Libhalog
// Author: likai.root@gmail.com

#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "hal_base_log.h"
#include "hal_error.h"

namespace libhalog {
namespace clib {

  HALLog::HALLog()
    : file_name_(NULL),
      fd_(STDOUT_FILENO),
      pos_(0),
      max_size_(INT64_MAX),
      switch_hour_(-1),
      switch_minute_(-1),
      level_filter_(&level_filter_default_),
      level_string_(&level_string_default_),
      redirect_std_(false),
      check_file_exist_(false) {
    pthread_rwlock_init(&file_name_lock_, NULL);
  }

  HALLog::~HALLog() {
    if (-1 != fd_ &&
        STDOUT_FILENO != fd_) {
      close(fd_);
      fd_ = -1;
    }
    if (NULL != file_name_) {
      free((void*)file_name_);
      file_name_ = NULL;
    }
    pthread_rwlock_destroy(&file_name_lock_);
  }

  int HALLog::set_file_name(const char *file_name) {
    int ret = HAL_SUCCESS;
    const char *tmp_file_name = NULL;
    if (NULL == file_name) {
      ret = HAL_INVALID_PARAM;
    } else if (NULL == (tmp_file_name = strdup(file_name))) {
      ret = HAL_ALLOCATE_FAIL;
    } else {
      pthread_rwlock_wrlock(&file_name_lock_);
      if (NULL != file_name_) {
        free((void*)file_name_);
        file_name_ = NULL;
      }
      file_name_ = tmp_file_name;
      pthread_rwlock_unlock(&file_name_lock_);
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

  int HALLog::set_redirect_std() {
    int ret = HAL_SUCCESS;
    redirect_std_ = true;
    return ret;
  }

  int HALLog::set_check_file_exist(const bool check_file_exist) {
    int ret = HAL_SUCCESS;
    check_file_exist_ = check_file_exist;
    return ret;
  }

  int HALLog::get_currnet_fd_(const int64_t reserve_size) {
    int ret = fd_;
    if (NULL != file_name_) {
      if (STDOUT_FILENO == fd_) {
        int tmp_fd = open(file_name_, O_RDWR | O_CREAT | O_APPEND, LOG_FILE_MODE);
        if (-1 == tmp_fd) {
          fprintf(stderr, "open log file [%s] fail, errno=%u\n", file_name_, errno);
        } else if (STDOUT_FILENO != __sync_val_compare_and_swap(&fd_, STDOUT_FILENO, tmp_fd)) {
          close(tmp_fd);
        } else {
          // open succ
        }
      }
    }
    return ret;
  }

}
}

