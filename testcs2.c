#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"

#define BUFLENGTH 100

int main(int argc, char *argv[]){
  int targetFile = open("README", O_RDONLY);

  int checkedFile = open("testcs2.txt", O_CHECKED | O_CREATE | O_WRONLY);

  printf(1, "Convert normal file to checked file and validate checksum: %s\n", "README");

  uchar data[BUFLENGTH];
  uchar checksum = 0;
  int code;
  while( (code = read(targetFile, data, BUFLENGTH)) > 0 ){
    //printf(1, "%s", data);
    for(int i = 0; i < code; i++)
      checksum = checksum ^ data[i];
    write(checkedFile, data, code);
  }
  printf(1, "True checksum: %d\n", checksum);

  struct stat filestat;
  fstat(checkedFile, &filestat);
  printf(1, "File checksum: %d\n", filestat.checksum);

  if( filestat.checksum == checksum){
    printf(1, "Test PASSED\n");
  }else{
    printf(1, "Test FAILED\n");
  }

  close(targetFile);
  close(checkedFile);

  exit();
}
