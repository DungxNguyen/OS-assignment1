#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"
#include "pstat.h"

int busywait(int ticks);

int
main(int argc, char *argv[])
{
 int pid = fork();
  if (pid == 0){
    setpri(2);
    int i = 0;
    for (i = 0; i < 20; i++){
      printf(0, "High priority: %d \n", i);
      busywait(10);
    }
  } else {
    int i = 0;
    for (i = 0; i < 20; i++){
      printf(0, "Low priority: %d \n", i);
      busywait(10);
    }
  }

  if (pid != 0){
    struct pstat p_stat; 

    getpinfo(&p_stat);

    for (int i = 0; i < NPROC; i++){
      if (p_stat.inuse[i] == 0)
        continue;
      printf(0, "ptable:%d pid:%d name:%s high:%d low:%d\n", i,  p_stat.pid[i], p_stat.pname[i],
             p_stat.hticks[i], p_stat.lticks[i]);
    }
    printf(1, "This test demos 1 high and 1 low processes\n");
    wait();
  }

  exit();
}

int busywait(int ticks){
  int cur_tick = uptime();
  while (uptime() < cur_tick + ticks){}

  return 0;
}
