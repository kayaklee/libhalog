// Libhalog
// Author: likai.root@gmail.com

#include <unistd.h>
#include "clib/hal_fixed_queue.h"
#include "clib/hal_util.h"
#include "clib/hal_base_log.h"
#include <gtest/gtest.h>

using namespace libhalog;
using namespace libhalog::clib;

struct QueueValue {
  int64_t a;
  int64_t b;
  int64_t sum;
  QueueValue() : a(0), b(0), sum(0) {};
};

struct GConf {
  HALFixedQueue<QueueValue> queue;
  int64_t loop_times;
  int64_t producer_count;
  GConf(const int64_t count) : queue(count) {}
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

void *thread_consumer(void *data) {
  set_cpu_affinity();
  GConf *g_conf = (GConf*)data;
  QueueValue stack_value;
  bool skip = false;
  while (true) {
    if (HAL_SUCCESS == g_conf->queue.pop(stack_value)) {
#ifndef DO_NOT_CHECK
      assert((stack_value.a + stack_value.b) == stack_value.sum);
#endif
      skip = false;
    } else {
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

void *thread_producer(void *data) {
  set_cpu_affinity();
  GConf *g_conf = (GConf*)data;
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
    while (HAL_SUCCESS != g_conf->queue.push(stack_value)) {}
  }
  __sync_add_and_fetch(&(g_conf->producer_count), -1);
  return NULL;
}

void run_test(GConf *g_conf, const int64_t thread_count) {
  pthread_t *pds_consumer = new pthread_t[thread_count];
  pthread_t *pds_producer = new pthread_t[thread_count];
  g_conf->producer_count = thread_count;
  int64_t timeu = get_cur_microseconds_time();
  for (int64_t i = 0; i < thread_count; i++) {
    pthread_create(&pds_consumer[i], NULL, thread_consumer, g_conf);
    pthread_create(&pds_producer[i], NULL, thread_producer, g_conf);
  }
  for (int64_t i = 0; i < thread_count; i++) {
    pthread_join(pds_consumer[i], NULL);
    pthread_join(pds_producer[i], NULL);
  }
  timeu = get_cur_microseconds_time() - timeu;
  fprintf(stdout, "threads=%ld+%ld push+pop=%ld timeu=%ld tps=%0.2f\n",
      thread_count, thread_count, thread_count * 2 * g_conf->loop_times, timeu, 1000000.0 * (double)(thread_count * 2 * g_conf->loop_times) / (double)(timeu));
  delete[] pds_producer;
  delete[] pds_consumer;
}

int main(const int argc, char **argv) {
  int64_t cpu_count = 0;
  if (1 < argc) {
    cpu_count = atoi(argv[1]);
  }
  if (0 >= cpu_count) {
    cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  }
  int64_t producer_count = (cpu_count + 1) / 2;

  int64_t memory = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
  int64_t available = memory * 4 / 10;
  int64_t count = available / (sizeof(QueueValue) + 8) / producer_count;

  GConf g_conf(count / 100);
  g_conf.loop_times = count;

#ifdef DO_NOT_CHECK
  fprintf(stdout, "Run without check pop result...\n");
#else
  fprintf(stdout, "Run and check pop result...\n");
#endif
  run_test(&g_conf, producer_count);
}
