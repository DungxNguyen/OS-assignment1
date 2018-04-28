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
  if (pid == 0) {
    setpri(2);
    sleep(2);
    int pri = 2;
    for (int i = 0; i < 30; i++){
      if( pri == 2)
        printf(1, "High priority 1: %d \n", i);
      else
        printf(1, "High priority 1 - becomes Low priority: %d\n", i);
      busywait(15);
      if( i == 10 ){
        pri = 1;
        setpri(1);
      }
    }

    struct pstat p_stat; 

    getpinfo(&p_stat);

    for (int i = 0; i < NPROC; i++){
      if (p_stat.inuse[i] == 0)
        continue;
      printf(0, "ptable:%d pid:%d name:%s high:%d low:%d\n", i,  p_stat.pid[i], p_stat.pname[i],
             p_stat.hticks[i], p_stat.lticks[i]);
    }
    printf(1, "There are 2 High Priority processes run parrallely in the beginning. One of them then is set to lower priority and run parallely with the other Lows\n");
  } else{
    int pid2 = fork();
    if (pid2 == 0) {
      setpri(2);
      int i = 0;
      for (i = 0; i < 20; i++){
        printf(1, "High priority 2: %d \n", i);
        busywait(15);
      }
    } else {
      int i = 0;
      for (i = 0; i < 20; i++){
        printf(1, "Low priority: %d \n", i);
        busywait(15);
      }
      wait();
    }
    wait();
    
  }
  
  exit();
}

int busywait(int ticks){
  int cur_tick = uptime();
  while (uptime() < cur_tick + ticks){}

  return 0;
}
