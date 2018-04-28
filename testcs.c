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
  char filename[10] = "testcs.txt";
  int file = open(filename, O_CREATE | O_RDWR | O_CHECKED);


  char string[] = "aoeuJTNHTEOHUNHH*(&#@NTHJNTHJNDNHOENTUHTNHJNTHHEUG)(#@HTNHTETNHJ(*RTRHOUHR";

  printf(1, "Test write simple string to file:\n%s\n", string);

  printf(1, "Length: %d\n", write(file, string, strlen(string)));

  uchar checksum = 0;
  for(int i = 0; i < strlen(string); i++){
    checksum = checksum ^ string[i];
  }
  printf(1, "True checksum: %d\n", checksum);

  struct stat filestat;
  fstat(file, &filestat);
  printf(1, "File checksum: %d\n", filestat.checksum);

  if( filestat.checksum == checksum){
    printf(1, "Test PASSED\n");
  }else{
    printf(1, "Test FAILED\n");
  }

  close(file);

  exit();
}
