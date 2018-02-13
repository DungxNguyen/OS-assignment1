//
// Created by dungn on 2/12/18.
//

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

int test_simple_test_shared_pages(void);
int test_fork(void);
int test_shared_data_fork(void);
int test_shared_data(void);
int pid;

int main(int argc, char *argv[]){
    printf(1, "Test Shared Pages\n");

    if (test_simple_test_shared_pages() == 0) {
        printf(1, "Simple test access and count PASSED\n");
    } else {
        printf(1, "Simple test access and count FAILED\n");
    }

    if (test_fork() == 0) {
        printf(1, "Fork test PASSED\n");
    } else {
        printf(1, "Fork test FAILED\n");
        if (pid == 0){
            exit();
        }
    }
    pid = 0;

    if (test_shared_data_fork() == 0) {
        printf(1, "Shared data fork test PASSED\n");
    } else {
        printf(1, "Shared data fork test FAILED\n");
        if (pid == 0){
            exit();
        }
    }
    pid = 0;


    if (test_shared_data() == 0) {
        printf(1, "Shared data test PASSED\n");
    } else {
        printf(1, "Shared data test FAILED\n");
    }


    exit();
}

int test_simple_test_shared_pages(void){
    printf(1, "Simple test access and count\n");


    // Record the original count
    int shmem_count_at_beginning[MAX_SHARED_PAGES];
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        shmem_count_at_beginning[i] = shmem_count(i);
//        printf(1, "%d\n", shmem_count_at_beginning[i]);
    }

    // All count must be +1
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        shmem_access(i);
//        printf(1, "Here: %d\n", shmem_count(i));
        if (shmem_count(i) != shmem_count_at_beginning[i] + 1) {
            return -1;
        }
    }

    // The second access, all count unchanged
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        shmem_access(i);
        if (shmem_count(i) != shmem_count_at_beginning[i] + 1) {
            return -1;
        }
    }
    return 0;
}

int test_fork(void){

    printf(1, "Fork test\n");

    // Record the original count
    int shmem_count_at_beginning[MAX_SHARED_PAGES];
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        shmem_count_at_beginning[i] = shmem_count(i);
    }

    // Fork new process
    pid = fork();

    wait();

    if (pid == 0){
        // This is fork process
        // The fork will see the count +1 from the beginning
        for (int i = 0; i < MAX_SHARED_PAGES; i++) {
            if (shmem_count(i) != shmem_count_at_beginning[i] + 1) {
                return -1;
            }
        }
        exit();
    } else {
        // This is main process
        // After fork dies, the main will see the count unchanged
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        shmem_access(i);
        if (shmem_count(i) != shmem_count_at_beginning[i]) {
            return -1;
        }
    }

    }
    return 0;
}

int test_shared_data_fork(void){
    printf(1, "Shared data fork test\n");

    int values[4] = {1000, 2000, 3000, 4000};


    // Main writes original data into shared pages
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        int *ptr = shmem_access(i);
        *ptr = values[i];
//        printf(1, "Main: %x:%d\n", ptr, *ptr);
    }

    pid = fork();

    wait();

    if (pid == 0){
        // This is fork process
        // It get the shared data, changes the values of original data
        for (int i = 0; i < MAX_SHARED_PAGES; i++) {
            int *ptr = shmem_access(i);
//            printf(1, "Fork: %x:%d\n", ptr, *ptr);
            *ptr += i;
        }
        exit();
    }

    // This is main process
    // It check the data after changed by fork process
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        int *ptr = shmem_access(i);
//        printf(1, "Main: %x:%d\n", ptr, *ptr);
        if (*ptr != values[i] + i) {
            return -1;
        }
    }


    return 0;
}

int test_shared_data(void){
    printf(1, "Shared data test\n");

    int values[4] = {1000, 2000, 3000, 4000};


    // Main writes original data into shared pages
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        int *ptr = shmem_access(i);
        *ptr = values[i];
    }

    pid = fork();

    wait();

    if (pid == 0){
        // The 1st fork process makes the 1st changes to the data
        for (int i = 0; i < MAX_SHARED_PAGES; i++) {
            int *ptr = shmem_access(i);
//            printf(1, "Fork: %x:%d\n", ptr, *ptr);
            *ptr += i;
        }

        int c_pid = getpid();

        fork();

        wait();

        if (c_pid != getpid()) {
            // The 2nd fork process makes 2nd changes to the data
            // after the 1st one

            for (int i = 0; i < MAX_SHARED_PAGES; i++) {
                int *ptr = shmem_access(i);
//            printf(1, "Fork: %x:%d\n", ptr, *ptr);
                *ptr += i;
            }

            exit();
        }

        exit();
    }

    // The main process need to check if its child and grand-child
    // both make changes to the data
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        int *ptr = shmem_access(i);
//        printf(1, "Main: %x:%d\n", ptr, *ptr);
        // 2 * i because forks changes the data 2 times
        if (*ptr != values[i] + 2 * i) {
            return -1;
        }
    }


    return 0;
}

