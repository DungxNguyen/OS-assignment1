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
  struct stat *filestat = malloc(sizeof(struct stat));
  int file = open(argv[1], O_RDONLY | O_CHECKED);

  fstat(file, filestat);

  printf(1, "%d %x %x %d %d %d\n", filestat->type, filestat->size, filestat->ino, filestat->nlink, filestat->dev, filestat->checksum);

  exit();
}
