// Libhalog
// Author: likai.root@gmail.com

#include <unistd.h>
#include <semaphore.h>
#include "clib/hal_hazard_version.h"
#include "clib/hal_util.h"
#include <gtest/gtest.h>

using namespace libhalog;
using namespace libhalog::clib;

template <typename T>
class LockFreeQueue;

template <typename T>
class FIFONode : public HALHazardNodeI {
  friend class LockFreeQueue<T>;
  public:
    FIFONode() : next_(NULL) {}
    FIFONode(const T &v) : next_(NULL), v_(v) {}
    ~FIFONode() {};
  public:
    void retire() {memset(&v_, 0, sizeof(v_));}//{ delete this; }
  public:
    void set_next(FIFONode *next) { next_ = next; }
    FIFONode *get_next() const { return next_; }
  public:
    T get_v() { return v_; }
  private:
    FIFONode *next_;
    T v_;
};

template <typename T>
class LockFreeQueue {
  typedef FIFONode<T> Node;
  public:
    LockFreeQueue() : hazard_version_(), head_(NULL) {
      head_ = new Node();
      tail_ = head_;
    }
    ~LockFreeQueue() {
      while (NULL != head_) {
        Node *tmp = head_;
        head_ = head_->get_next();
        tmp->retire();
      }
    }
  public:
    void push(const T &v, Node *node) {
#ifdef DO_NOT_CHECK
      UNUSED(v);
#else
      node = new(node) Node(v);
#endif
      uint64_t handle = 0;
      hazard_version_.acquire(handle);
      Node *curr = ATOMIC_LOAD(&tail_);
      Node *old = curr;
      while (old != (curr = __sync_val_compare_and_swap(&tail_, old, node))) {
        old = curr;
      }
      curr->set_next(node);
      hazard_version_.release(handle);
    }
    bool pop(T &v) {
      bool bret = false;
      uint64_t handle = 0;
      hazard_version_.acquire(handle);
      Node *curr = ATOMIC_LOAD(&head_);
      Node *old = curr;
      Node *node = curr->get_next();
      while (NULL != node
          && old != (curr = __sync_val_compare_and_swap(&head_, old, node))) {
        old = curr;
        node = curr->get_next();
      }
      if (NULL != node) {
#ifdef DO_NOT_CHECK
        UNUSED(v);
#else
        v = node->get_v();
#endif
        hazard_version_.add_node(curr);
        bret = true;
      }
      hazard_version_.release(handle);
      return bret;
    }
  private:
    HALHazardVersion hazard_version_;
    Node *head_ CACHE_ALIGNED;
    Node *tail_ CACHE_ALIGNED;
};

struct QueueValue {
  int64_t a;
  int64_t b;
  int64_t sum;
  QueueValue() : a(0), b(0), sum(0) {};
};

struct GConf {
  LockFreeQueue<QueueValue> queue;
  int64_t loop_times;
  int64_t producer_count;

  pthread_mutex_t mutex;
  pthread_cond_t cond;
  sem_t sem;
};

void set_cpu_affinity() {
  int64_t cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(gettn() % cpu_count, &cpuset);
  if (0 == pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset)) {
    //LOG_INFO(CLIB, "pthread_setaffinity_np succ %ld", gettn() % cpu_count);
  } else {
    LOG_WARN(CLIB, "pthread_setaffinity_np fail %ld", gettn() % cpu_count);
  }
}

void *pthread_consumer(void *data) {
  set_cpu_affinity();
  GConf *g_conf = (GConf*)data;
  QueueValue stack_value;
  while (true) {
    if (g_conf->queue.pop(stack_value)) {
#ifndef DO_NOT_CHECK
      assert((stack_value.a + stack_value.b) == stack_value.sum);
#endif
    } else {
      pthread_mutex_lock(&g_conf->mutex);
      if (g_conf->queue.pop(stack_value)) {
        pthread_mutex_unlock(&g_conf->mutex);
#ifndef DO_NOT_CHECK
        assert((stack_value.a + stack_value.b) == stack_value.sum);
#endif
      } else {
        if (0 == ATOMIC_LOAD(&(g_conf->producer_count))) {
          break;
        }
        pthread_cond_wait(&g_conf->cond, &g_conf->mutex);
        pthread_mutex_unlock(&g_conf->mutex);
      }
    }
  }
  return NULL;
}

void *sem_consumer(void *data) {
  set_cpu_affinity();
  GConf *g_conf = (GConf*)data;
  QueueValue stack_value;
  bool skip = false;
  while (true) {
    sem_wait(&g_conf->sem);
    if (g_conf->queue.pop(stack_value)) {
#ifndef DO_NOT_CHECK
      assert((stack_value.a + stack_value.b) == stack_value.sum);
#endif
      skip = false;
    } else {
      sem_post(&g_conf->sem);
      if (0 == ATOMIC_LOAD(&(g_conf->producer_count))) {
        if (skip) {
          break;
        } else {
          skip = true;
        }
      }
    }
  }
  return NULL;
}

void *pthread_producer(void *data) {
  set_cpu_affinity();
  GConf *g_conf = (GConf*)data;
  FIFONode<QueueValue> *nodes = new FIFONode<QueueValue>[g_conf->loop_times];
#ifndef DO_NOT_CHECK
  int64_t sum_base = gettn() * g_conf->loop_times;
#endif
  QueueValue stack_value;
  for (int64_t i = 0; i < g_conf->loop_times; i++) {
#ifndef DO_NOT_CHECK
    stack_value.a = sum_base + i;
    stack_value.b = i;
    stack_value.sum = sum_base + 2*i;
#endif
    pthread_mutex_lock(&g_conf->mutex);
    g_conf->queue.push(stack_value, &nodes[i]);
    pthread_cond_signal(&g_conf->cond);
    pthread_mutex_unlock(&g_conf->mutex);
  }
  pthread_mutex_lock(&g_conf->mutex);
  __sync_add_and_fetch(&(g_conf->producer_count), -1);
  pthread_cond_signal(&g_conf->cond);
  pthread_mutex_unlock(&g_conf->mutex);
  return NULL;
}

void *sem_producer(void *data) {
  set_cpu_affinity();
  GConf *g_conf = (GConf*)data;
  FIFONode<QueueValue> *nodes = new FIFONode<QueueValue>[g_conf->loop_times];
#ifndef DO_NOT_CHECK
  int64_t sum_base = gettn() * g_conf->loop_times;
#endif
  QueueValue stack_value;
  for (int64_t i = 0; i < g_conf->loop_times; i++) {
#ifndef DO_NOT_CHECK
    stack_value.a = sum_base + i;
    stack_value.b = i;
    stack_value.sum = sum_base + 2*i;
#endif
    g_conf->queue.push(stack_value, &nodes[i]);
    sem_post(&g_conf->sem);
  }
  __sync_add_and_fetch(&(g_conf->producer_count), -1);
  sem_post(&g_conf->sem);
  return NULL;
}

void run_test(GConf *g_conf, const int64_t thread_count, const bool use_sem) {
  pthread_t *pds_consumer = new pthread_t[thread_count];
  pthread_t *pds_producer = new pthread_t[thread_count];
  g_conf->producer_count = thread_count;
  int64_t timeu = get_cur_microseconds_time();
  for (int64_t i = 0; i < thread_count; i++) {
    if (use_sem) {
      pthread_create(&pds_consumer[i], NULL, sem_consumer, g_conf);
      pthread_create(&pds_producer[i], NULL, sem_producer, g_conf);
    } else {
      pthread_create(&pds_consumer[i], NULL, pthread_consumer, g_conf);
      pthread_create(&pds_producer[i], NULL, pthread_producer, g_conf);
    }
  }
  for (int64_t i = 0; i < thread_count; i++) {
    pthread_join(pds_consumer[i], NULL);
    pthread_join(pds_producer[i], NULL);
  }
  timeu = get_cur_microseconds_time() - timeu;
  fprintf(stdout, "use_sem=%s threads=%ld+%ld push+pop=%ld timeu=%ld tps=%0.2f\n",
      use_sem ? "true" : "false", thread_count, thread_count, thread_count * 2 * g_conf->loop_times, timeu, 1000000.0 * (double)(thread_count * 2 * g_conf->loop_times) / (double)(timeu));
  delete[] pds_producer;
  delete[] pds_consumer;
}

int main(const int argc, char **argv) {
  int use_sem = 0;
  int64_t cpu_count = 0;
  if (1 < argc) {
    use_sem = atoi(argv[1]);
  }
  if (2 < argc) {
    cpu_count = atoi(argv[2]);
  }
  if (0 >= cpu_count) {
    cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  }
  int64_t producer_count = (cpu_count + 1) / 2;

  int64_t memory = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
  int64_t available = memory * 4 / 10;
  int64_t count = available / sizeof(FIFONode<QueueValue>) / producer_count;

  GConf g_conf;
  g_conf.loop_times = count;
  pthread_mutex_init(&g_conf.mutex, NULL);
  pthread_cond_init(&g_conf.cond, NULL);
  sem_init(&g_conf.sem, 0, 0);

#ifdef DO_NOT_CHECK
  fprintf(stdout, "Run without check pop result...\n");
#else
  fprintf(stdout, "Run and check pop result...\n");
#endif
  run_test(&g_conf, producer_count, 0 != use_sem);
}
