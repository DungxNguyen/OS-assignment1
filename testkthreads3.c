/*
This test case demonstrates when a thread exits without joining its children
Expected behavior: all thread A's children will be passed to A's parent.
A's parent then can join the unjoined children from A.
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

kthread_t grandchild;

void thread_child(void* arg)
{
  grandchild = thread_create(thread_grandchild, &GRANDCHILD);
  exit();
}

void thread_grandchild(void* arg)
{
  sleep(200);
  exit();
}

int main(void)
{
  kthread_t child = thread_create(thread_child, &CHILD);
  thread_join(child);
  if (thread_join(grandchild) == grandchild.pid) 
    printf(1, "Test PASSED\n");
  else
    printf(1, "Test FAILED\n");
  
  exit();
}
