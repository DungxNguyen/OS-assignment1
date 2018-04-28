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
    int i = 0;
    int pri = 2;
    for (i = 0; i < 30; i++){
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
