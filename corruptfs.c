#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>

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

int findDir(uint address, char* name);
int checkBmap(uint address);

int checkBadInode(); //1
int checkBadAddressInInode(); //2
int checkRootDir(); //3
int checkDirFormat(); //4
int checkParentDir(); //5
int checkUsedInodeBitmap(); //6
int checkUsedBitmap(); //7
int checkAddressInUsedOnce(); //8
int checkInodeInDirectory();//9
int checkInodeMarkFree();//10
int checkReferenceCountForFile();//11
int checkDirAppearOnce();//12


int main(int argc, char *argv[]){
  uchar buf[BSIZE];
  fsfd = open(argv[1], O_RDWR);
  if(fsfd < 0){
    perror(argv[1]);
    printf("image not found\n");
    exit(1);
  }

  srand(time(NULL));   // should only be called once

  rsect(1, buf);
  memmove(&sb, buf, sizeof(sb));

  //printf("%d %d %d %d %x %x %x\n", sb.size, sb.nblocks, sb.ninodes, sb.nlog, sb.logstart, sb.inodestart, sb.bmapstart);

  nbitmap = sb.size / (BSIZE*8) + 1;
  ninodeblocks =  sb.ninodes / IPB + 1;
  nlog = sb.nlog;
  nmeta = 2 + nlog + ninodeblocks + nbitmap;
  nblocks = sb.size - nmeta;

  //printf("%d %d %d %d %d\n", nbitmap, ninodeblocks, nlog, nmeta, nblocks);

  //printf("\ncheck last bitmap: %d\n", checkBmap(sb.size - 1));
  if(argc < 2){
    printf("Please include an error type\n");
    exit(-1);
  }

  printf("-------------------------------Error type: %s\n", argv[2]);

  if(!strcmp(argv[2], "1"))
    checkBadInode(); //1
  else if(!strcmp(argv[2], "2"))
    checkBadAddressInInode(); //2
  else if(!strcmp(argv[2], "3"))
    checkRootDir(); //3
  else if(!strcmp(argv[2], "4"))
    checkDirFormat(); //4
  else if(!strcmp(argv[2], "5"))
    checkParentDir(); //5
  else if(!strcmp(argv[2], "6"))
    checkUsedInodeBitmap();//6
  else if(!strcmp(argv[2], "7"))
    checkUsedBitmap(); //7
  else if(!strcmp(argv[2], "8"))
    checkAddressInUsedOnce();//8
  else if(!strcmp(argv[2], "9"))
    checkInodeInDirectory();//9
  else if(!strcmp(argv[2], "10"))
    checkInodeMarkFree();//10
  else if(!strcmp(argv[2], "11"))
    checkReferenceCountForFile();//11
  else if(!strcmp(argv[2], "12"))
    checkDirAppearOnce();//12
  else
    printf("Please include an error type");

  close(fsfd);
  exit(0);
}
int checkParentDir(){//5
  int findChild = 0;
  for(uint inode = 0; inode < sb.ninodes -1; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    //printf("%d ", dInode.type);
    if (dInode.type == T_DIR) {
      for(uint inode2 = inode + 1; inode2 < sb.ninodes; inode2++) {
        struct dinode dInode2;
        rinode(inode2, &dInode2);
        //if(inode2 == 23){
        //  printf("23 parent %d %d\n", iInode2.addrs[0], findDir(dInode2.addrs[0], ".."));
        //}
        if (dInode2.type == T_DIR && findDir(dInode2.addrs[0], "..") ==inode) {
          findChild = 1;
          uchar buf[BSIZE];
          rsect(dInode.addrs[0], buf);
          struct dirent* dir = (struct dirent *) buf;
          for(int j = 0; j < BSIZE / sizeof(struct dirent); j++){
            if (dir[j].inum == inode2){
              //printf("E12 %d %d \n", inode, inode2);
              dir[j].inum = 0; 
              wsect(dInode.addrs[0], buf);
              return 0;
            }
          }
        }
      }
    }
  }
  if(!findChild){
    printf("ERROR When create Error 5: There is only 1 dir, no parent-children relationship\n");
    exit(1);
  }
  
  return 0;
}

void checkInodeInDirectoryRecursive(struct dinode dir, int *inodeRealUse){
  if (dir.type != T_DIR) 
    return;

  for(int dBlock = 0; dBlock < NDIRECT; dBlock++) {
    if(dir.addrs[dBlock] == 0)
      continue;
    struct dirent dirEntry;
    uchar buf[BSIZE];
    rsect(dir.addrs[dBlock], buf);
    for (int i = 0; i < BSIZE / sizeof(struct dirent); i++) {
      memmove(&dirEntry, buf + i * sizeof(struct dirent), sizeof(struct dirent));
      if (dirEntry.inum == 0 || !strcmp(dirEntry.name, ".") || !strcmp(dirEntry.name, ".."))
        continue;
      inodeRealUse[dirEntry.inum] += 1;
      struct dinode childDir;
      rinode(dirEntry.inum, &childDir);
      checkInodeInDirectoryRecursive(childDir, inodeRealUse);
    }
  }

  if(dir.addrs[NDIRECT] == 0)
    return;

  uchar buf[BSIZE];
  rsect(dir.addrs[NDIRECT], buf);
  uint *indirectPointer = (uint*)buf;
  for(int indirect = 0; indirect < NINDIRECT; indirect++){
    if(indirectPointer[indirect] == 0)
      continue;
    struct dirent dirEntry;
    uchar buf[BSIZE];
    rsect(indirectPointer[indirect], buf);
    for (int i = 0; i < BSIZE / sizeof(struct dirent); i++) {
      memmove(&dirEntry, buf + i * sizeof(struct dirent), sizeof(struct dirent));
      if (dirEntry.inum == 0 || !strcmp(dirEntry.name, ".") || !strcmp(dirEntry.name, ".."))
        continue;
      inodeRealUse[dirEntry.inum] += 1;
      struct dinode childDir;
      rinode(dirEntry.inum, &childDir);
      checkInodeInDirectoryRecursive(childDir, inodeRealUse);
    }
  }

}

int checkReferenceCountForFile(){//11
  struct dinode root;
  rinode(1, &root);
  int inodeRealUse[sb.ninodes];
  for(int i = 0; i < sb.ninodes; i++){
    inodeRealUse[i] = 0;
  }
  inodeRealUse[1] = 1;

  checkInodeInDirectoryRecursive(root, inodeRealUse);

  for(uint inode = 3; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    //printf("%d ", dInode.type);
    if (dInode.type == T_FILE) {
      dInode.nlink += 1;
      winode(inode, &dInode);
      return 0;
    }
  }
  
  return 0;
}

int checkDirAppearOnce(){//12
  for(uint inode = 0; inode < sb.ninodes -1; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    //printf("%d ", dInode.type);
    if (dInode.type == T_DIR) {
      for(uint inode2 = inode + 1; inode2 < sb.ninodes; inode2++) {
        struct dinode dInode2;
        rinode(inode2, &dInode2);
        //if(inode2 == 23){
        //  printf("23 parent %d %d\n", iInode2.addrs[0], findDir(dInode2.addrs[0], ".."));
        //}
        if (dInode2.type == T_DIR && findDir(dInode2.addrs[0], "..") !=inode) {
          uchar buf[BSIZE];
          rsect(dInode2.addrs[0], buf);
          struct dirent* dir = (struct dirent *) buf;
          for(int j = 0; j < BSIZE / sizeof(struct dirent); j++){
            if (dir[j].inum == 0){
              //printf("E12 %d %d \n", inode, inode2);
              dir[j].inum = inode; 
              wsect(dInode2.addrs[0], buf);
              return 0;
            }
          }
        }
      }
    }
  }
  
  return 0;
}

int checkInodeMarkFree(){//10
  struct dinode root;
  rinode(1, &root);
  int inodeRealUse[sb.ninodes];
  for(int i = 0; i < sb.ninodes; i++){
    inodeRealUse[i] = 0;
  }
  inodeRealUse[1] = 1;

  checkInodeInDirectoryRecursive(root, inodeRealUse);

  for(int i = 5; i < sb.ninodes; i++){
    if(!inodeRealUse[i]){
      for(uint inode = 0; inode < sb.ninodes; inode++) {
        struct dinode dInode;
        rinode(inode, &dInode);
        //printf("%d ", dInode.type);
        if (dInode.type == T_DIR){
          uchar buf[BSIZE];
          rsect(dInode.addrs[0], buf);
          struct dirent* dir = (struct dirent *) buf;
          for(int j = 0; j < BSIZE / sizeof(struct dirent); j++){
            if (dir[j].inum == 0){
              dir[j].inum = i; 
              wsect(dInode.addrs[0], buf);
              return 0;
            }
          }
        }
      }
    }
  }
        
  
  return 0;
}

int checkInodeInDirectory(){//9
  struct dinode root;
  rinode(1, &root);
  int inodeRealUse[sb.ninodes];
  for(int i = 0; i < sb.ninodes; i++){
    inodeRealUse[i] = 0;
  }
  inodeRealUse[1] = 1;

  checkInodeInDirectoryRecursive(root, inodeRealUse);

  for(uint inode = 5; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    //printf("%d ", dInode.type);
    if (dInode.type == 0) {
      dInode.type = 1;
      winode(inode, &dInode);
    }
  }
  
  return 0;
}

int findDir(uint address, char* name){
  struct dirent dir;
  uchar buf[BSIZE];
  rsect(address, buf);
  for (int i = 0; i < BSIZE / sizeof(struct dirent); i++) {
    memmove(&dir, buf + i * sizeof(struct dirent), sizeof(struct dirent));
    if (dir.inum == 0)
      continue;
    //printf("%d %s\n", dir.inum, dir.name);
    if (strcmp(name, dir.name) == 0){
      //For error 12
      //dir.inum = 0;
      return dir.inum;
    }
  }
  return 0;
}

int findDirByInum(uint address, ushort inum){
  struct dirent dir;
  uchar buf[BSIZE];
  rsect(address, buf);
  for (int i = 0; i < BSIZE / sizeof(struct dirent); i++) {
    memmove(&dir, buf + i * sizeof(struct dirent), sizeof(struct dirent));
    if (dir.inum == 0)
      continue;
    //printf("%d %s\n", dir.inum, dir.name);
    if (dir.inum == inum){
      return 1;
    }
  }
  return 0;
}

int checkParentDirAtAddress(uint address){
  int curDirInode = findDir(address, "."); 
  int parentDirInode = findDir(address, "..");
  struct dinode parentDir;

  rinode(parentDirInode, &parentDir);
  if (parentDir.type != T_DIR) 
    return 0;

  for(int dBlock = 0; dBlock < NDIRECT; dBlock++) {
    if(parentDir.addrs[dBlock] == 0)
      continue;
    if(findDirByInum(parentDir.addrs[dBlock], curDirInode))
      return 1;
  }

  if(parentDir.addrs[NDIRECT] == 0)
    return 0;

  uchar buf[BSIZE];
  rsect(parentDir.addrs[NDIRECT], buf);
  uint *indirectPointer = (uint*)buf;
  for(int indirect = 0; indirect < NINDIRECT; indirect++){
    if(indirectPointer[indirect] == 0)
      continue;
    if(findDirByInum(indirectPointer[indirect], curDirInode))
      return 1;
  }

  return 0;
}

//int checkParentDir(){ //5
//  uint inode;
//  for(inode = 0; inode < sb.ninodes; inode++) {
//    struct dinode dInode;
//    rinode(inode, &dInode);
//    if (dInode.type == T_DIR) {
//      for(int dBlock = 0; dBlock < NDIRECT; dBlock++) {
//        // no map is okay
//        if(dInode.addrs[dBlock] == 0)
//          continue;
//        if(!checkParentDirAtAddress(dInode.addrs[dBlock])){
//          printf("parent directory mismatch.\n");
//          close(fsfd);
//          exit(1);
//        }
//      }
//      if(dInode.addrs[NDIRECT] == 0)
//        continue;
//      uchar buf[BSIZE];
//      rsect(dInode.addrs[NDIRECT], buf);
//      uint *indirectPointer = (uint*)buf;
//      for(int indirect = 0; indirect < NINDIRECT; indirect++){
//        if(indirectPointer[indirect]== 0)
//          continue;
//        if(!checkParentDirAtAddress(indirectPointer[indirect])){
//          printf("parent directory mismatch.\n");
//          close(fsfd);
//          exit(1);
//        }
//      }
//    }
//  }
//  return 0;
//}

int checkDirFormat(){ //4
  uint inode;
  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    if (dInode.type == T_DIR) {
      for(int dBlock = 0; dBlock < NDIRECT; dBlock++) {
        // no map is okay
        if(dInode.addrs[dBlock] == 0)
          continue;
        uchar buf[BSIZE];
        rsect(dInode.addrs[dBlock], buf);
        struct dirent* dir = (struct dirent*) buf;
        for (int i = 0; i < BSIZE / sizeof(struct dirent); i++) {
          //memmove(&dir, buf + i * sizeof(struct dirent), sizeof(struct dirent));
          if (dir[i].inum == 0)
            continue;
          //printf("%d %s\n", dir.inum, dir.name);
          if (strcmp(".", dir[i].name) == 0){
            printf("I remove: %d\n", inode);
            dir[i].inum = 0;
            wsect(dInode.addrs[dBlock], buf);
            return 0;
          }
        }
      }
    }
  }
  return 0;
}

int checkBmap(uint address){
  uchar buf[BSIZE];
  rsect(address/(8 * BSIZE) + sb.bmapstart, buf);
  //for(int i = 0; i < BSIZE; i++){
  //  printf("%d ", buf[i]);
  //}

  return (buf[address % (8 * BSIZE) / 8] >> (address % 8)) % 2;
}

int checkRootDir(){
  struct dinode dInode;
  rinode(1, &dInode);
  dInode.type = T_FILE;
  winode(1, &dInode);
  return 0;
}

int checkBadInode(){ //1
  // check Error 1
  uint inode = rand() % sb.ninodes;
  struct dinode dInode;
  rinode(inode, &dInode);
  dInode.type = -1;
  winode(inode, &dInode);
  return 0;
}

int checkBadAddressInInode(){//2
  // check Error 1
  uint inode = rand() % sb.ninodes;
  struct dinode dInode;
  rinode(inode, &dInode);
  dInode.type = T_FILE;
  if (rand()%2){
    dInode.addrs[0] = sb.size;
  }else{
    dInode.addrs[0] = nmeta - 1;
  }
  winode(inode, &dInode);
  return 0;
}

int checkUsedInodeBitmap(){ //6
  uint inode;
  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    if (dInode.type == 0)
      continue;
    for(int dBlock = 0; dBlock <= NDIRECT; dBlock++) {
      uint address = dInode.addrs[dBlock];
      uchar buf[BSIZE];
      rsect(address/(8 * BSIZE) + sb.bmapstart, buf);
      buf[address % (8 * BSIZE) / 8] = 0;
      wsect(address/(8 * BSIZE) + sb.bmapstart, buf);
    }
    
  }
  return 0;
}

int checkUsedBitmap(){ //7
  uchar buf[BSIZE];
  rsect(sb.bmapstart, buf);
  for(int i = 0; i < BSIZE; i++){
    if(buf[i] != 255){
      buf[i] = 255;
    }
  }
  wsect(sb.bmapstart, buf);
  return 0;
}

int checkAddressInUsedOnce(){ //8
  uint inode;
  uint addressToCopy = 0;
  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    if (dInode.type == 0)
      continue;
    for(int dBlock = 0; dBlock <= NDIRECT; dBlock++) {
      //printf("%d\n", dInode.addrs[dBlock] - nmeta);
      if(dInode.addrs[dBlock] != 0 && !addressToCopy){
        addressToCopy = dInode.addrs[dBlock];
        break;
      }else if(dInode.addrs[dBlock] == 0){
        dInode.addrs[dBlock] = addressToCopy;
        winode(inode, &dInode);
        return 0;
      }
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
