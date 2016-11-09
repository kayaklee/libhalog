// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_FIXED_QUEUE_H__
#define __HAL_CLIB_FIXED_QUEUE_H__
#include <stdint.h>
#include <assert.h>

#include "clib/hal_error.h"
#include "clib/hal_atomic.h"
#include "clib/hal_malloc.h"
#include "clib/hal_mod_define.h"

namespace libhalog {
namespace clib {
  template <class T>
  class HALFixedQueue {
    struct Node {
      T data_;
      uint64_t seq_;
    };
    public:
      HALFixedQueue(const int64_t length);
      ~HALFixedQueue();
    public:
      int push(const T &data);
      int pop(T &data);
      int64_t size() const;
    private:
      int64_t length_;
      Node *nodes_ CACHE_ALIGNED;
      uint64_t producer_ CACHE_ALIGNED;
      uint64_t consumer_ CACHE_ALIGNED;
  };

  template <class T>
  HALFixedQueue<T>::HALFixedQueue(const int64_t length)
    : length_(0),
      nodes_(NULL),
      producer_(0), 
      consumer_(0) {
    assert(0 < length);
    nodes_ = (Node*)hal_malloc(sizeof(Node) * length, HALModIds::FIXED_QUEUE);
    assert(NULL != nodes_);
    length_ = length;
    memset(nodes_, -1, sizeof(Node) * length);
  }

  template <class T>
  HALFixedQueue<T>::~HALFixedQueue() {
    if (NULL != nodes_) {
      hal_free(nodes_);
      nodes_ = NULL;
    }
  }

  template <class T>
  int HALFixedQueue<T>::push(const T &data) {
    int ret = HAL_QUEUE_FULL;
    uint64_t old_pos = producer_;
    uint64_t cmp_pos = old_pos;
    uint64_t new_pos = old_pos + 1;
    while (true) {
      uint64_t index = old_pos % length_;
      if ((uint64_t)-1 != nodes_[index].seq_
          && old_pos == producer_) {
        break;
      }
      if (cmp_pos == (old_pos = __sync_val_compare_and_swap(&producer_, cmp_pos, new_pos))) {
        nodes_[index].data_ = data;
        nodes_[index].seq_ = old_pos;
        ret = HAL_SUCCESS;
        break;
      } else {
        cmp_pos = old_pos;
        new_pos = old_pos + 1;
      }
    }
    return ret;
  }

  template <class T>
  int HALFixedQueue<T>::pop(T &data) {
    int ret = HAL_QUEUE_EMPTY;
    uint64_t old_pos = consumer_;
    uint64_t cmp_pos = old_pos;
    uint64_t new_pos = old_pos + 1;
    while (true) {
      uint64_t index = old_pos % length_;
      if (old_pos != nodes_[index].seq_
          && old_pos == consumer_) {
        break;
      }
      if (cmp_pos == (old_pos = __sync_val_compare_and_swap(&consumer_, cmp_pos, new_pos))) {
        data = nodes_[index].data_;
        nodes_[index].seq_ = -1;
        ret = HAL_SUCCESS;
        break;
      } else {
        cmp_pos = old_pos;
        new_pos = old_pos + 1;
      }
    }
    return ret;
  }

  template <class T>
  int64_t HALFixedQueue<T>::size() const {
    return producer_ - consumer_;
  }

}
}

#endif // __HAL_CLIB_FIXED_QUEUE_H__
