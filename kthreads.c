#include "types.h"
#include "param.h"
#include "kthreads.h"
#include "user.h"
#include "mmu.h"
#include "x86.h"

struct kthread thread_create(void (*start_routine)(void*), void *arg){
  struct kthread new_thread;

  void *stack = malloc(PGSIZE * 2);

  stack = (void*)(stack - (uint)stack % PGSIZE + PGSIZE);

  new_thread.pid = clone(start_routine, arg, stack);

  return new_thread;
}

void thread_join(struct kthread thread){
  join(thread.pid);
}

void lock_acquire(volatile lock_t * lock){
  while (xchg(lock, 1) == 1){
  }
}

void lock_release(volatile lock_t * lock){
  while (xchg(lock, 0) == 0){
  }
}

void init_lock(lock_t * lock){
  *lock = 0;
}
