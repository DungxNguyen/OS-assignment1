//
// Created by dungn on 2/10/18.
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

void simple_null_test(void);
void fork_test(void);
void null_pointer_param_test(void);


char *argv[] = { "0" };

int main(int argc, char *argv[]){
    printf(1, "Null Test Program\n");

    fork_test();

    null_pointer_param_test();

    simple_null_test();

    exit();
}


void simple_null_test(void){
    int *ptr = 0;

    printf(1, "Simple Null Test\n");

    printf(1, "Pointer: %d\n", ptr);

    printf(1, "Dereference pointer. Trap %d expected.\n", T_PGFLT);

    printf(1, "Dereference: %d\n", *ptr);

    printf(1, "Simple null test FAILED\n");
}

// DUNGN
// THe fork test copy from forktest
void fork_test(void) {
    int N = 1000;
    int n, pid;

    printf(1, "Fork test\n");

    for(n=0; n<N; n++){
        pid = fork();
        if(pid < 0)
            break;
        if(pid == 0)
            exit();
    }

    if(n == N){
        printf(1, "fork claimed to work N times!\n", N);
        exit();
    }

    for(; n > 0; n--){
        if(wait() < 0){
            printf(1, "wait stopped early\n");
            exit();
        }
    }

    if(wait() != -1){
        printf(1, "wait got too many\n");
        exit();
    }

    printf(1, "Fork test PASSED\n");
}

void null_pointer_param_test(void){
    printf(1, "Null param test\n");

    // DUNGN
    // Pass a void pointer to the system call
    if (exec((void *) 0, argv) < 0){
        printf(1, "Null param PASSED\n");
        return;
    }

    printf(1, "Null param FAILED\n");
}
