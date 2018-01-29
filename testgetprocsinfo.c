//
// Created by dungn on 1/28/18.
//

#include "types.h"
#include "user.h"
#include "param.h"
#include "procinfo.h"

int
main(int argc, char *argv[])
{

    printf(1, "Hello XV6\n");

//    printf(1, "Process Count: %d\n", 0);
////    printf(1, "Hello \n");
////    printf(1, "Process Count: %d\n", processcount);

    struct procinfo procinfos[NPROC];
    int c = getprocsinfo(procinfos);

    printf(1, "Count: %d\n", c);

    int i = 0;
    for(i = 0; i < c; i++) {
        printf(1, "%d %s\n", procinfos[i].pid, procinfos[i].pname);
    }

    exit();
}

