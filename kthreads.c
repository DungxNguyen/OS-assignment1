#include "types.h"
#include "param.h"
#include "kthreads.h"
#include "user.h"
#include "mmu.h"
#include "x86.h"

lock_t s_lock = 0;

struct kthread thread_create(void (*start_routine)(void*), void *arg){
  struct kthread new_thread;

  lock_acquire(&s_lock);
  void *stack = malloc(PGSIZE * 2);

  //printf(1, "Malloc for %d: %x \n", *(int *) arg, (uint) stack);
  stack = (void*)((uint)stack - (uint)stack % PGSIZE + PGSIZE);

  //printf(1, "Stack for %d: %x \n", *(int *) arg, (uint) stack);
  lock_release(&s_lock);

  new_thread.pid = clone(start_routine, arg, stack);

  return new_thread;
}

int thread_join(struct kthread thread){
  return join(thread.pid);
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
