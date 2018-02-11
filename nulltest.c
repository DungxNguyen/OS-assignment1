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

int main(int argc, char *argv[]){
    printf(1, "Null Test\n");

    int* ptr = (void *)0;

    printf(1, "Pointer: %d\n", ptr);

    fork();

    wait();

    printf(1, "Folk successfully\n");

    printf(1, "Dereference: %d\n", *ptr);

    exit();
}
