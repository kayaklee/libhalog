// Libhalog
// Author: likai.root@gmail.com

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hal_mod_define.h"
#include "hal_base_log.h"
#include "hal_util.h"

namespace libhalog {
namespace clib {

  HALModItem::HALModItem()
    : mod_id_(-1),
      mod_str_(NULL) {
    memset(alloc_size_, 0, sizeof(alloc_size_));
    memset(free_size_, 0, sizeof(free_size_));
    memset(alloc_count_, 0, sizeof(alloc_count_));
    memset(free_count_, 0, sizeof(free_count_));
  }

  HALModItem::~HALModItem() {
  }

  void HALModItem::set_mod(const int mod_id, const char *mod_str) {
    mod_id_ = mod_id;
    mod_str_ = mod_str;
  }

  void HALModItem::on_alloc(const int64_t size) {
    int64_t tn = gettn() % HAL_MAX_TSI_COUNT;
    alloc_size_[tn] += size;
    alloc_count_[tn]++;
  }

  void HALModItem::on_free(const int64_t size) {
    int64_t tn = gettn() % HAL_MAX_TSI_COUNT;
    free_size_[tn] += size;
    free_count_[tn]++;
  }

  const char *HALModItem::cstring() {
    static __thread char BUFFERS[HAL_MAX_TSI_COUNT][MAX_STR_LENGTH];
    static int64_t pos = 0;
    char *buffer = BUFFERS[pos++ % HAL_MAX_TSI_COUNT];
    int64_t alloc_size = 0;
    int64_t alloc_count = 0;
    int64_t hold_size = 0;
    int64_t hold_count = 0;
    for (int64_t i = 0; i < HAL_MAX_TSI_COUNT; i++) {
      alloc_size += alloc_size_[i];
      alloc_count += alloc_count_[i];
      hold_size += (alloc_size_[i] - free_size_[i]);
      hold_count += (alloc_count_[i] - free_count_[i]);
    }
    snprintf(buffer, MAX_STR_LENGTH, "[%04d][%s] hold_size=%ld hold_count=%ld alloc_size=%ld alloc_count=%ld",
        mod_id_, mod_str_, hold_size, hold_count, alloc_size, alloc_count);
    return buffer;
  }

  void HALModSet::print_usage() {
    LOG_INFO(CLIB, "========== MOD Usage Start ==========");
    for (int64_t i = 0; i < ARRAYSIZE(mods_); i++) {
       LOG_INFO(CLIB, "%s", mods_[i].cstring());
    }
    LOG_INFO(CLIB, "========== MOD Usage End ==========");
  }

}
}

