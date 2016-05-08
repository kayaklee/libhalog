// Libhalog
// Author: likai.root@gmail.com

#include <assert.h>
#include "hal_page_arena.h"
#include "hal_base_log.h"

namespace libhalog {
namespace clib {

  HALPageArena::HALPageArena(const int64_t page_size, IHALAllocator &allocator, const int mod_id)
    : page_size_(page_size),
      allocator_(allocator),
      mod_id_(mod_id),
      free_page_list_(NULL),
      normal_page_list_(NULL),
      big_page_list_(NULL),
      used_(0),
      total_(0),
      count_(0) {
    assert(page_size > (int64_t)sizeof(pagearena::NormalPage));
    assert(page_size > (int64_t)sizeof(pagearena::BigPage));
  }

  HALPageArena::~HALPageArena() {
    reset();
  }

  pagearena::NormalPage *HALPageArena::get_normal_page_(const int64_t alloc_size) {
    pagearena::NormalPage *page = normal_page_list_;
    if (NULL != page
        && (page->pos + alloc_size) <= page_size_) {
      // do nothing
    } else {
      if (NULL != free_page_list_) {
        page = free_page_list_;
        free_page_list_ = free_page_list_->next;
      } else {
        if (NULL == (page = (pagearena::NormalPage*)allocator_.alloc(sizeof(*page) + page_size_, mod_id_))) {
          LOG_WARN(CLIB, "allocate fail, size=%ld", page_size_);
        } else {
          total_ += (sizeof(*page) + page_size_);
        }
      }
      if (NULL != page) {
        page->pos = 0;
        page->next = normal_page_list_;
        normal_page_list_ = page;
      }
    }
    return page;
  }

  pagearena::BigPage *HALPageArena::get_big_page_(const int64_t page_size) {
    pagearena::BigPage *page = (pagearena::BigPage*)allocator_.alloc(sizeof(*page) + page_size, mod_id_);
    if (NULL == page) {
      LOG_WARN(CLIB, "allocate fail, size=%ld", page_size);
    } else {
      page->next = big_page_list_;
      big_page_list_ = page;
      total_ += (sizeof(*page) + page_size);
    }
    return page;
  }

  void *HALPageArena::alloc(const int64_t size) {
    char *ret = NULL;
    if (size > page_size_) {
      pagearena::BigPage *page = get_big_page_(size);
      if (NULL != page) {
        ret = page->buf;
      }
    } else {
      pagearena::NormalPage *page = get_normal_page_(size);
      if (NULL != page) {
        ret = page->buf + page->pos;
        page->pos += (int32_t)size;
      }
    }
    if (NULL != ret) {
      used_ += size;
      count_++;
    }
    return ret;
  }

  void HALPageArena::reuse() {
    used_ = 0;
    count_ = 0;
    total_ = 0;
    while (NULL != big_page_list_) {
      pagearena::BigPage *tmp = big_page_list_;
      big_page_list_ = tmp->next;
      allocator_.free(tmp);
    }
    pagearena::NormalPage *iter = normal_page_list_;
    while (NULL != iter) {
      pagearena::NormalPage *next = iter->next;
      total_ += (sizeof(*iter) + page_size_);
      iter->pos = 0;
      iter->next = free_page_list_;
      free_page_list_ = iter;
      iter = next;
    }
    normal_page_list_ = NULL;
  }

  void HALPageArena::reset() {
    used_ = 0;
    count_ = 0;
    total_ = 0;
    while (NULL != big_page_list_) {
      pagearena::BigPage *tmp = big_page_list_;
      big_page_list_ = tmp->next;
      allocator_.free(tmp);
    }
    while (NULL != normal_page_list_) {
      pagearena::NormalPage *tmp = normal_page_list_;
      normal_page_list_ = tmp->next;
      allocator_.free(tmp);
    }
    while (NULL != free_page_list_) {
      pagearena::NormalPage *tmp = free_page_list_;
      free_page_list_ = tmp->next;
      allocator_.free(tmp);
    }
  }

  int64_t HALPageArena::used() const {
    return used_;
  }

  int64_t HALPageArena::total() const {
    return total_;
  }

  int64_t HALPageArena::count() const {
    return count_;
  }

}
}

