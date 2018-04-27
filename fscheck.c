
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

int checkBadInode(); //1
int checkBadAddressInInode(); //2
int checkRootDir(); //3
int checkDirFormat(); //4
int checkParentDir(); //5
int checkUsedInodeBitmap(); //6
int checkUsedBitmap(); //7
int checkAddressInUsedOnce(); //8


int main(int argc, char *argv[]){
  uchar buf[BSIZE];
  fsfd = open(argv[1], O_RDONLY);
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

  checkBadInode(); //1
  checkBadAddressInInode(); //2
  checkRootDir(); //3

  close(fsfd);
  exit(0);
}

int checkRootDir(){
  struct dinode dInode;
  rinode(1, &dInode);
  if(dInode.type != T_DIR){
    printf("%d\n", dInode.type);
    printf("ERROR MESSAGE: root directory does not exist.\n");
    close(fsfd);
    exit(1);
  }
  return 0;
}

int checkBadInode(){
  // check Error 1
  uint inode;
  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    //printf("%d ", dInode.type);
    if (dInode.type != 0 && dInode.type != T_FILE && dInode.type != T_DIR && dInode.type != T_DEV) {
      printf("ERROR: bad inode.\n");
      close(fsfd);
      exit(1);
    }
  }
  return 0;
}

int checkBadAddressInInode(){
  uint inode;
  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    if (dInode.type != 0) {
      for(int dBlock = 0; dBlock <= NDIRECT; dBlock++) {
        // no map is okay
        if(dInode.addrs[dBlock] == 0)
          continue;
        // map to address bigger than maximum or smaller than starting data block
        if(dInode.addrs[dBlock] >= sb.size || dInode.addrs[dBlock] < nmeta){
          printf("ERROR: bad address in inode.\n");
          close(fsfd);
          exit(1);
        }
      }
      if(dInode.addrs[NDIRECT] == 0)
        continue;
      uchar buf[BSIZE];
      rsect(dInode.addrs[NDIRECT], buf);
      uint *indirectPointer = (uint*)buf;
      for(int indirect = 0; indirect < NINDIRECT; indirect++){
        if(indirectPointer[indirect]!= 0 && (indirectPointer[indirect] >= sb.size || indirectPointer[indirect] < nmeta)){
          printf("ERROR: bad address in inode.\n");
          close(fsfd);
          exit(1);
        }
      }
    }
  }
  return 0;
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

void
winode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
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

uint
ialloc(ushort type)
{
  uint inum = freeinode++;
  struct dinode din;

  bzero(&din, sizeof(din));
  din.type = xshort(type);
  din.nlink = xshort(1);
  din.size = xint(0);
  winode(inum, &din);
  return inum;
}

void
balloc(int used)
{
  uchar buf[BSIZE];
  int i;

  printf("balloc: first %d blocks have been allocated\n", used);
  assert(used < BSIZE*8);
  bzero(buf, BSIZE);
  for(i = 0; i < used; i++){
    buf[i/8] = buf[i/8] | (0x1 << (i%8));
  }
  printf("balloc: write bitmap block at sector %d\n", sb.bmapstart);
  wsect(sb.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void
iappend(uint inum, void *xp, int n)
{
  char *p = (char*)xp;
  uint fbn, off, n1;
  struct dinode din;
  char buf[BSIZE];
  uint indirect[NINDIRECT];
  uint x;

  rinode(inum, &din);
  off = xint(din.size);
  // printf("append inum %d at off %d sz %d\n", inum, off, n);
  while(n > 0){
    fbn = off / BSIZE;
    assert(fbn < MAXFILE);
    if(fbn < NDIRECT){
      if(xint(din.addrs[fbn]) == 0){
        din.addrs[fbn] = xint(freeblock++);
      }
      x = xint(din.addrs[fbn]);
    } else {
      if(xint(din.addrs[NDIRECT]) == 0){
        din.addrs[NDIRECT] = xint(freeblock++);
      }
      rsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      if(indirect[fbn - NDIRECT] == 0){
        indirect[fbn - NDIRECT] = xint(freeblock++);
        wsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      }
      x = xint(indirect[fbn-NDIRECT]);
    }
    n1 = min(n, (fbn + 1) * BSIZE - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * BSIZE), n1);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  winode(inum, &din);
}
