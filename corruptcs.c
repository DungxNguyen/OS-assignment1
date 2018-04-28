
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"

#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

#define NINODES 200

void balloc(int);
void wsect(uint, void*);
void winode(uint, struct dinode*);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type);
void iappend(uint inum, void *p, int n);

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]

int nbitmap; 
int ninodeblocks;
int nlog = LOGSIZE;
int nmeta;    // Number of meta blocks (boot, sb, nlog, inode, bitmap)
int nblocks;  // Number of data blocks

int fsfd;
struct superblock sb;
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;

// convert to intel byte order
ushort
xshort(ushort x)
{
  ushort y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint
xint(uint x)
{
  uint y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}


int corruptcs();

int main(int argc, char *argv[]){
  uchar buf[BSIZE];
  fsfd = open(argv[1], O_RDWR);
  if(fsfd < 0){
    perror(argv[1]);
    printf("image not found\n");
    exit(1);
  }

  rsect(1, buf);
  memmove(&sb, buf, sizeof(sb));

  printf("%d %d %d %d %x %x %x\n", sb.size, sb.nblocks, sb.ninodes, sb.nlog, sb.logstart, sb.inodestart, sb.bmapstart);

  nbitmap = sb.size / (BSIZE*8) + 1;
  ninodeblocks =  sb.ninodes / IPB + 1;
  nlog = sb.nlog;
  nmeta = 2 + nlog + ninodeblocks + nbitmap;
  nblocks = sb.size - nmeta;

  printf("%d %d %d %d %d\n", nbitmap, ninodeblocks, nlog, nmeta, nblocks);

  //printf("\ncheck last bitmap: %d\n", checkBmap(sb.size - 1));

  corruptcs();

  close(fsfd);
  exit(0);
}

int corruptcs(){

  uint inode;
  uchar buf[BSIZE] = "WRONG DATA";

  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    if (dInode.type == T_CHECKED)
      for(int dBlock = 0; dBlock < NDIRECT; dBlock++) { // Or just need 1 block
        if (dInode.addrs[dBlock] == 0)
          continue;
        printf("write corrupt data\n");
        wsect(dInode.addrs[dBlock] & 0x00ffffff, buf);
      }
  }

  return 0;
}

void
rinode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, BSIZE) != BSIZE){
    perror("read");
    exit(1);
  }
}


void
wsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek");
    exit(1);
  }
  if(write(fsfd, buf, BSIZE) != BSIZE){
    perror("write");
    exit(1);
  }
}
