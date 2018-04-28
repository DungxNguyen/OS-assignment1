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
  printf(1, "Read from checked file and compare to the original file\n");
  int targetFile = open("README", O_RDONLY);

  int checkedFile = open("testcs2.txt", O_RDONLY);
  if(checkedFile == -1){
    printf(1, "Please run testcs2 first to create a checked file\n");
    close(targetFile);
    close(checkedFile);
    exit();
  }


  char data1[BUFLENGTH+1];
  char data2[BUFLENGTH+1];
  data1[BUFLENGTH] = '\0';
  data2[BUFLENGTH] = '\0';
  int code1, code2;
  while( (code2 = read(targetFile, data2, BUFLENGTH)) > 0 &&
         (code1 = read(checkedFile, data1, BUFLENGTH)) > 0 ){
    if(code1 != code2){
      printf(1, "Test FAILED\n");
      close(targetFile);
      close(checkedFile);
      exit();
    }

    for(int i = 0; i < code1; i++)
      if(data1[i] != data2[i]){
        printf(1, "Test FAILED\n");
        close(targetFile);
        close(checkedFile);
        exit();
      }

    printf(1, "%d %d %d\n", code1, code2, strcmp( data1, data2));
  }

  printf(1, "Test PASSED\n");

  close(targetFile);
  close(checkedFile);

  exit();
}
