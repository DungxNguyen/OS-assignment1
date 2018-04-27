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

int printPStat(void);

int
main(int argc, char *argv[])
{
  printPStat();
  
  exit();
}

int
printPStat(void){
  struct pstat p_stat; 

  getpinfo(&p_stat);

  int i = 0;
  for (i = 0; i < NPROC; i++){
    if (p_stat.inuse[i] == 0)
      continue;
    printf(0, "ptable:%d pid:%d name:%s high:%d low:%d\n", i,  p_stat.pid[i], p_stat.pname[i],
           p_stat.hticks[i], p_stat.lticks[i]);
  }

  return 0;
}
