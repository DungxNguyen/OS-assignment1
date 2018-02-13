//
// Created by dungn on 2/12/18.
//

#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"

//int test_large_heap();

int main(){
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        int* ptr = shmem_access(i);
        int old_value = *ptr;
        *ptr += 1;
        printf(1, "%d:%d:%d\n", shmem_count(i), old_value, *ptr );
    }

//    printf(1, "%x\n", shmem_access(0));
//    printf(1, "%x\n", shmem_access(1));


    exit();
}

//int test_large_heap(){
//   char* a = sbrk(0);
//    sbrk(KERNBASE - (uint)a - 4096 * 2 );
//    return 0;
//}