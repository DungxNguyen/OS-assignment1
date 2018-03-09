/*
This test case demonstrate multi-level of thread. 
Expected behavior: child thread will create grandchild
                   grandchild creates great_grandchild
                   great_grandchild is the only one cane change value of 
                   can_great_grand_child_access to 1
 */


#include "types.h"
#include "stat.h"
#include "user.h"
#include "kthreads.h"

#define LOCKS_ON 1
#define NULL 0

void thread_child(void* arg);
void thread_grandchild(void* arg);
void thread_great_grandchild(void* arg);

// global lock for product
lock_t lock;
// our product
int CHILD = 1111;
int GRANDCHILD = 2222;

int can_great_grand_child_access = 0;

void thread_child(void* arg)
{
  kthread_t grandchild = thread_create(thread_grandchild, &GRANDCHILD);
  thread_join(grandchild);
  exit();
}

void thread_grandchild(void* arg)
{
  kthread_t great_grandchild = thread_create(thread_great_grandchild, &GRANDCHILD);
  thread_join(great_grandchild);
  exit();
}

void thread_great_grandchild(void* arg)
{
  can_great_grand_child_access = 1;
  exit();
}

int main(void)
{
  kthread_t child = thread_create(thread_child, &CHILD);
  thread_join(child);
  if (can_great_grand_child_access == 1)
    printf(1, "Test PASSED\n");
  else
    printf(1, "Test FAILED\n");
  exit();
}
