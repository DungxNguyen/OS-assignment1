//
// Created by dungn on 1/28/18.
//

#include "types.h"
#include "user.h"
#include "param.h"
#include "procinfo.h"


void test1(void);
void test2(void);
void test3(void);

int
main(int argc, char *argv[])
{

    printf(1, "Hello XV6\n");

//    printf(1, "Process Count: %d\n", 0);
////    printf(1, "Hello \n");
////    printf(1, "Process Count: %d\n", processcount);

    struct procinfo procinfos[NPROC];
    int c = getprocsinfo(procinfos);

    printf(1, "Process Count: %d\n", c);

    int i = 0;
    for(i = 0; i < c; i++) {
        printf(1, "%d %s\n", procinfos[i].pid, procinfos[i].pname);
    }

    printf(1, "Begin test\n");
    test1();
    test2();
    test3();

    exit();
}


void
test1(void)
{
    struct procinfo procinfos[NPROC];

    int pcount = getprocsinfo(procinfos);

//    char *argv[] = {};
    printf(1, "test 1\n");

//    if(exec("echo", argv) < 0){
//        printf(1, "test 1 failed");
//        exit();
//    }

    if (getprocsinfo(procinfos) == pcount) {
        printf(1, "test 1 passed\n");
    }
    else
        printf(1, "test 1 failed\n");
}


void
test2(void)
{
    struct procinfo procinfos[NPROC];

    int pcount = getprocsinfo(procinfos);

//    char *argv[] = {};
    printf(1, "test 2\n");

    int pid = fork();

//    if(exec("echo", argv) < 0){
//        printf(1, "test 1 failed");
//        exit();
//    }

    if (getprocsinfo(procinfos) == pcount + 1) {
        printf(1, "test 2 passed\n");
    }
    else
        printf(1, "test 2 failed\n");

    kill(pid);
    wait();
}


void
test3(void)
{
    struct procinfo procinfos[NPROC];

    int pcount = getprocsinfo(procinfos);

//    char *argv[] = {};
    printf(1, "test 3\n");

    int pid = fork();

//    if(exec("echo", argv) < 0){
//        printf(1, "test 1 failed");
//        exit();
//    }

    kill(pid);
    wait();

    if(getpid() == pid){
        exit();
    }

    if (getprocsinfo(procinfos) == pcount) {
        printf(1, "test 3 passed\n");
    }
    else
        printf(1, "test 3 failed\n");

}

