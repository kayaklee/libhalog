// Libhalog
// Author: likai.root@gmail.com

#include <unistd.h>
#include "clib/hal_hazard_version.h"
#include "clib/hal_util.h"
#include <gtest/gtest.h>

using namespace libhalog;
using namespace libhalog::clib;

template <typename T>
class LIFONode : public HALHazardNodeI {
  public:
    LIFONode(const T &v) : next_(NULL), v_(v) {};
    ~LIFONode() {};
  public:
    void retire() { delete this; }
  public:
    void set_next(LIFONode *next) { next_ = next; }
    LIFONode *get_next() const { return next_; }
  public:
    T get_v() { return v_; }
  private:
    LIFONode *next_;
    T v_;
};

template <typename T>
class LockFreeStack {
  typedef LIFONode<T> Node;
  public:
    LockFreeStack() : hazard_version_(), top_(NULL) {}
    ~LockFreeStack() {
      while (NULL != top_) {
        Node *tmp = top_;
        top_ = top_->get_next();
        tmp->retire();
      }
    }
  public:
    void push(const T &v) {
      Node *node = new Node(v);
      Node *curr = ATOMIC_LOAD(&top_);
      Node *old = curr;
      node->set_next(old);
      while (old != (curr = __sync_val_compare_and_swap(&top_, old, node))) {
        old = curr;
        node->set_next(old);
      }
    }
    bool pop(T &v) {
      bool bret = false;
      uint64_t handle = 0;
      hazard_version_.acquire(handle);
      Node *curr = ATOMIC_LOAD(&top_);
      Node *old = curr;
      if (NULL != curr) {
        Node *next = curr->get_next();
        while (old != (curr = __sync_val_compare_and_swap(&top_, old, next))) {
          old = curr;
          next = curr->get_next();
        }
        v = curr->get_v();
        hazard_version_.add_node(curr);
        bret = true;
      }
      hazard_version_.release(handle);
      return bret;
    }
  private:
    HALHazardVersion hazard_version_;
    Node *top_;
};

int main() {
  LockFreeStack<int64_t> s;
  s.push(1024);
  int64_t v = 0;
  s.pop(v);
  assert(v == 1024);
  assert(!s.pop(v));
}
