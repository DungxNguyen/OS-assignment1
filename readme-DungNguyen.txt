1. Test cases:

I checked with your original test and it passed. My modified tests are:

-testkthreads :
pretty much like your test, but each consumer call a producer thread, which ends up we have 5 producers at different levels (3 are child threads, 2 are grand child threads)

-testkthreads2 : 
This test case demonstrate multi-level of thread. 
Expected behavior: child thread will create grandchild
                   grandchild creates great_grandchild
                   great_grandchild is the only one cane change value of 
                   can_great_grand_child_access to 1

-testkthreads3 :
This test case demonstrates when a thread exits without joining its children
Expected behavior: all thread A's children will be passed to A's parent.
A's parent then can join the unjoined children from A.

-testkthreads4 :
This test demonstrate when a thread doesn't call exit()
Expected behavior: a trap will be called. but the parent of the thread can 
                   still join the unexited grand child thread. The program does not hang.

-testkthreads5 :
This test demonstrate when main process doesn't call join()
Expected behavior: When main process exit, all of its threads will be killed.
                   The test program should exit successfully, after that there is no zombie
                   (check by Ctrl-P)



2. Implementations:

--------function clone in proc.c---------------

It's pretty much similar with clone, but rather than copy pgdir, new process uses the same pgdir,
The new stack is defined as below:

  // Set instruction pointer
  np->tf->eip = (uint) fcn;

  // Set stack pointer:
  np->tf->esp = (uint) stack + PGSIZE - 2 * sizeof(uint); 
  *(uint*)np->tf->esp = 0xffffffff;
  *(uint*)(np->tf->esp + sizeof(uint)) = (uint) arg;

-------function join in proc.c -------------------

Also, it's almost the same with original wait, except that i do not delete pgdir of a thread

      if(p->parent != curproc || p->pid != pid)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        kfree(p->kstack);
        p->kstack = 0;
        // freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->thread = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }

-------function proc_grow in proc.c ---------------------

When a thread is grown, all other process that shares the same virtual space will be updated the
new capacity

  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pgdir == curproc->pgdir){
      p->sz = sz;
    }
  }


-------function wait in proc.c ------------------------------

In wait, check if the process to be waited is the last reference to a pgdir. If it is, free the pgdir

      int is_p_the_last_reference = 1;
      if(p->state == ZOMBIE){
        // Check if p is the last reference of its family tree
        struct proc *p2;
        for(p2 = ptable.proc; p2 < &ptable.proc[NPROC]; p2++){
          if(p2->state != UNUSED && p2->pgdir == p->pgdir) {
            // p is not the last reference
            is_p_the_last_reference = 0;
          }
        }
	...
          // when p is the last ref, free the memory
          freevm(p->pgdir);
        }


-------function exit in proc.c--------------------

Modified code are long. In summary, when a child thread exits, it passes all children to its parent.
When a process exits, all children (grand/great grand...) which shares the same address space are
killed but not passed to initproc like processes.

-------Lock----------------------------------

void lock_acquire(volatile lock_t * lock){
  while (xchg(lock, 1) == 1){
  }
}

void lock_release(volatile lock_t * lock){
  while (xchg(lock, 0) == 0){
  }
}
