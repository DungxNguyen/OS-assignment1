/*
  This test demonstrate when main process doesn't call join()
  Expected behavior: When main process exit, all of its threads will be killed  
                     
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
  //while(1);
  exit();
}

int main(void)
{
  thread_create(thread_child, &CHILD);
  exit();
}
