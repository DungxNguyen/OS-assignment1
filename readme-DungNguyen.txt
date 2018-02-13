The source code should be compiled and executed as normal:
1. make
2. make qemu

PART A----------------------------------------------------

In order to make the 1st page of all program to be invalid, I put this segment
of code to exec.c

  if((sz = allocuvm(pgdir, sz, sz + PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - PGSIZE));

It will be executed before copying program instruction into memory. The function
"clearpteu" makes the 1st table inaccessible. Therefore, all access in user space to
the 1st page of virtual memory space will leads to a invalid access.

The "fork" function also need to change. In particular, the function "copyuvm" in vm.c
has to be changed as:
     for(i = PGSIZE ; i < sz; i += PGSIZE){
rather than
     for(i = 0 ; i < sz; i += PGSIZE){
to skip the 1st table.

In syscall.c, to avoid null pointer as parameter, I modified the argptr to check if the
value of pointer is 0 (zero) or not. If it is, return -1 as error.

----------------------------------------------------------

Test:

The test program is "nulltest". It run 3 tests:
1. The fork test, to make sure fork works as expected
2. Null pointer parameter test. The system call should return -1 when it is passed an
null pointer as parameter.
3. Null pointer dereference: when the user program try to dereference a null pointer,
the expected behavior is to trap the process and kill it. The trap should has code 14 (PAGE_FAULT).
* Note: the test program is expected to hang in the last test


PART B----------------------------------------------------

In this part, I used 2 arrays in vm.c to store the shared pages and shared count:
uint shared_memory[MAX_SHARED_PAGES];
int shared_count[MAX_SHARED_PAGES];

The function shmem_access in vm.c return the physical addresses to each query.

I also add 2 field to process infomation (struct proc):
uint shared_memory[MAX_SHARED_PAGES]; // to store the shared pages virtual addresses
int shared_memory_count; // to count the shared pages of each process

I comment most of the changes in the source code with tag "DUNGN". Major changes are in those functions:

vm.c:
	shmem_access
	shmem_count
	shmem_init
	copyuvm
	freevm_process

proc.c:
	wait
	fork

These changes are tagged with "DUNGN"

To solve several cases:
1. Large heap: in shmem_access, I check the size of the program. If the size is not sufficient for
a new page, the shmem_access return 0 as not successfully allocate shared pages.

2. Fork: in fork, the 2 fields in proc are copied to children process. The memory is also copied
by function "copyuvm", because the original version doesn't know the existence of this parts.
When the children process dies, freevm_process will take care the virtual space of the shared pages.

----------------------------------------------------------
The demo program for the shared pages is "increaseshared", it will print out:
- the count of processes using shared pages
- the current values of 4 1st values of 4 shared pages
- the new values of 4 1st values of 4 shared pages
$ increaseshared
1:0:1
1:0:1
1:0:1
1:0:1

The test program for the shared page is "testsharedpage" (testsharedpages.c). It run 4 tests:
1. Simple access and count. The process first request access to shared memory. After the 1st request,
the count should be increased by 1. After the 2nd request, the count should be unchanged.

2. Test fork. When a children process is created, the count should be increase by 1.
When the children process dies, the count then return to the original value. (decreased by 1)

3. Test shared fork. The children process tried to modified the shared pages' 1st integer values.
The parent process then should recognize the changes from the children process.

4. Test multiple forks. Similar to test 3, but in this test, the 1st children process fork a new
grand-children process and they make changes to the shared pages.

---
After running "testsharedpage", if you run "increaseshared" you will see the count will be
reset to 0 (+1 is the increaseshared). 
