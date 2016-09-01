// Libhalog
// Author: likai.root@gmail.com

#ifndef __HAL_CLIB_BTREE_H__
#define __HAL_CLIB_BTREE_H__
#include <stdint.h>
#include <pthread.h>

#include <algorithm>

#include "hal_error.h"
#include "hal_util.h"
#include "hal_spin_rwlock.h"
#include "hal_base_log.h"

#define BTREE "btree"

namespace libhalog {
namespace clib {
namespace btree {
  template <typename Key, typename Value, int64_t NodeSize>
  class BaseNode {
    typedef BaseNode<Key, Value, NodeSize> Node;
    struct KV {
      KV() {}
      KV(const Key &key) : key_(key) {}
      Key key_;
      Value value_;
    };
    class KeyCompare {
      public:
        KeyCompare(const Node &node) : node_(node) {}
        bool operator() (const KV &a, const Key &b) const;
        bool operator() (const Key &a, const KV &b) const;
      private:
        const Node &node_;
    };
    public:
      BaseNode();
      ~BaseNode();
    public:
      int search(const Key &key, int64_t &pos, bool &found) const;
      int get(const int64_t pos, Value &value) const;
    public:
      // update in place
      int update(const int64_t pos, const Key &key, const Value &value);
    public:
      // copy and modify
      int insert(Node *raid, const int64_t pos, const Key &key, const Value &value);
      int insert_and_split(Node *raid1, Node *raid2, const int64_t pos, const Key &key, const Value &value);
      int del(Node *raid, const int64_t pos, const Key &key);
      int merge(Node *raid, Node *mate, const bool forward);
    public:
      int64_t get_size() const;
      int get_max_key(Key &key) const;
      void release_lock();
    public:
      HALSpinRWLock cow_rwlock_;
      pthread_spinlock_t update_lock_;
      KV kvs_[NodeSize];
      int8_t size_;
  };
}
namespace btree {
  template <typename Key, typename Value, int64_t NodeSize>
  bool BaseNode<Key, Value, NodeSize>::KeyCompare::operator() (
      const KV &a,
      const Key &b) const {
    return a.key_ < b;
  }

  template <typename Key, typename Value, int64_t NodeSize>
  bool BaseNode<Key, Value, NodeSize>::KeyCompare::operator() (
      const Key &a,
      const KV &b) const {
    return a < b.key_;
  }

  template <typename Key, typename Value, int64_t NodeSize>
  BaseNode<Key, Value, NodeSize>::BaseNode()
    : cow_rwlock_(),
      size_(0) {
    pthread_spin_init(&update_lock_, PTHREAD_PROCESS_PRIVATE);
  }

  template <typename Key, typename Value, int64_t NodeSize>
  BaseNode<Key, Value, NodeSize>::~BaseNode() {
    pthread_spin_destroy(&update_lock_);
  }

  template <typename Key, typename Value, int64_t NodeSize>
  int BaseNode<Key, Value, NodeSize>::search(const Key &key, int64_t &pos, bool &found) const {
    int ret = HAL_SUCCESS;
    if (0 > size_) {
      LOG_WARN(BTREE, "unexpected size=%hhd this=%p", size_, this);
      ret = HAL_UNEXPECTED_ERROR;
    } else if (size_ == 0) {
      pos = 0;
      found = false;
    } else {
      const KV *p = std::lower_bound(kvs_, kvs_ + size_, key, KeyCompare(*this));
      pos = p - kvs_;
      if (key == p->key_) {
        found = true;
      } else {
        found = false;
      }
    }
    return ret;
  }

  template <typename Key, typename Value, int64_t NodeSize>
  int BaseNode<Key, Value, NodeSize>::get(const int64_t pos, Value &value) const {
    int ret = HAL_SUCCESS;
    if (0 > pos || pos > size_) {
      LOG_WARN(BTREE, "invalid pos=%ld size=%hhd this=%p", pos, size_, this);
      ret = HAL_INVALID_PARAM;
    } else {
      value = kvs_[pos].value_;
    }
    return ret;
  }
}
}
}

#endif // __HAL_CLIB_BTREE_H__
