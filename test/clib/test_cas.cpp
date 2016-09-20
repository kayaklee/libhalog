#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include "clib/hal_util.h"

using namespace libhalog;
using namespace libhalog::clib;

int64_t g CACHE_ALIGNED;
int64_t g1 CACHE_ALIGNED;
int64_t g2 CACHE_ALIGNED;
int64_t l = 10000000;

void atomic(int64_t &g, const int64_t v) {
  int64_t curr = ATOMIC_LOAD(&g);
  int64_t old = curr;
  while (old != (curr = __sync_val_compare_and_swap(&g, old, v + old))) {
    old = curr;
  }
}

void *thread_func(void *data) {
  UNUSED(data);
  int64_t base = gettn() * l;
  for (int64_t i = 0; i < l; i++) {
    atomic(g, base+i);
    //atomic(g1, base+i);
  }
  return NULL;
}

void run_test(const int64_t t) {
  g = 0;
  g1 = 0;
  g2 = 0;
  int64_t timeu = get_cur_microseconds_time();
  pthread_t *pds = new pthread_t[t];
  for (int64_t i = 0; i < t; i++) {
    pthread_create(&pds[i], NULL, thread_func, NULL);
  }
  for (int64_t i = 0; i < t; i++) {
    pthread_join(pds[i], NULL);
  }
  timeu = get_cur_microseconds_time() - timeu;
  fprintf(stdout, "t=%02ld timeu=%ld\ttps=%0.2f\n", t, timeu, 1000000.0 * (double)(t * l) / double(timeu));
  delete[] pds;
}

int main() {
  run_test(1);
  run_test(2);
  run_test(4);
  run_test(8);
  run_test(16);
  run_test(32);
}

