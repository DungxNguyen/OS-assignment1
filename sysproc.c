#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "procinfo.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_getprocsinfo(void){
  struct procinfo *procinfos;

  if(argptr(0, (void*)&procinfos, sizeof(*procinfos)) < 0)
    return -1;

  struct proc procs[NPROC];
  int nprocs = proclist(procs);
  for (int i = 0; i < nprocs; ++i) {
    procinfos[i].pid = procs[i].pid;
    safestrcpy(procinfos[i].pname, procs[i].name, strlen(procs[i].name) + 1);
//    struct procinfo pinfo;
//    pinfo.pid = procs[i].pid;
//    safestrcpy(pinfo.pname, procs[i].name, strlen(procs[i].name) + 1);
//    procinfos[i] = pinfo;
  }
  return nprocs;
}