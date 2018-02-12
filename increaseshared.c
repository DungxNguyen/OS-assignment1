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

int main(){
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        int* ptr = shmem_access(i);
        int old_value = *ptr;
        *ptr += 1;
        printf(1, "%d:%d\n", old_value, *ptr );
    }

    exit();
}
