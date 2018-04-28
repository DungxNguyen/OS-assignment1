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
  int file = open(argv[1], O_CREATE | O_RDWR | O_CHECKED);

  char string[10] = "aoeu";

  printf(1, "%d\n", write(file, string, 10));

  uchar checksum = 0;
  for(int i = 0; i < strlen(string); i++){
    checksum = checksum ^ string[i];
  }
  printf(1, "True checksum: %d\n", checksum);

  close(file);

  exit();
}
