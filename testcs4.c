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
  printf(1, "Read from corrupted checked file and report errors\n");
  printf(1, "NOTICE: Test only valid if the image is corrupt by ./corruptcs\n");
  printf(1, "Run testcs2 first, go to Linux console and run 'make corrupt' then restart VM\n");

  int checkedFile = open("testcs2.txt", O_RDONLY);
  if(checkedFile == -1){
    printf(1, "File not found, please run testcs2 first then corrupt the file\n");
    close(checkedFile);
    exit();
  }

  char buf[BUFLENGTH+1];
  int code;
  while((code = read(checkedFile, buf, BUFLENGTH)) != 0 ){
    if(code == -1){
      printf(1, "Test PASSED\n");
      close(checkedFile);
      exit();
    }
  }


  
  printf(1, "Test FAILED\n");
  printf(1, "Are you sure you run 'make corrupt' in Linux console and restart VM before running this test?\n");

  close(checkedFile);

  exit();
}
