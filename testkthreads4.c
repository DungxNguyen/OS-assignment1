/*
  This test demonstrate when child thread doesn't call exit()
  Expected behavior: a trap will be called. but the parent of the thread can 
                     still join the unexited child thread. The program does not hang.
 */
#include "types.h"
#include "stat.h"
#include "user.h"
#include "kthreads.h"

#define LOCKS_ON 1
#define NULL 0

void thread_child(void* arg);
void thread_grandchild(void* arg);

// global lock for product
lock_t lock;
// our product
int CHILD = 1111;
int GRANDCHILD = 2222;

void thread_child(void* arg)
{
}

int main(void)
{
  kthread_t child = thread_create(thread_child, &CHILD);

  sleep(200);

  if (thread_join(child) == child.pid)
    printf(1, "Test PASSED\n");
  else
    printf(1, "Test FAILED\n");
  exit();
}
