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

  printf("%d %d %d %d %d\n", nbitmap, ninodeblocks, nlog, nmeta, nblocks);

  //printf("\ncheck last bitmap: %d\n", checkBmap(sb.size - 1));

  checkBadInode(); //1
  checkBadAddressInInode(); //2
  checkRootDir(); //3
  checkDirFormat(); //4
  checkParentDir(); //5
  checkUsedInodeBitmap();//6
  checkUsedBitmap(); //7
  checkAddressInUsedOnce();//8
  checkInodeInDirectory();//9
  checkInodeMarkFree();//10
  checkReferenceCountForFile();//11
  checkDirAppearOnce();//12

  close(fsfd);
  exit(0);
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
      //printf("%d\n", dirEntry.inum);
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

  for(uint inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    //printf("%d ", dInode.type);
    if (dInode.type == T_FILE && inodeRealUse[(int) inode] != dInode.nlink) {
      printf("%d\n", inode);
      printf("bad reference count for file.\n");
      close(fsfd);
      exit(1);
    }
  }
  
  return 0;
}

int checkDirAppearOnce(){//12
  struct dinode root;
  rinode(1, &root);
  int inodeRealUse[sb.ninodes];
  for(int i = 0; i < sb.ninodes; i++){
    inodeRealUse[i] = 0;
  }
  inodeRealUse[1] = 1;

  checkInodeInDirectoryRecursive(root, inodeRealUse);

  for(uint inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    //printf("%d ", dInode.type);
    if (dInode.type == T_DIR && inodeRealUse[(int) inode] > 1) {
      printf("%d\n", inode);
      printf("directory appears more than once in file system.\n");
      close(fsfd);
      exit(1);
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

  for(uint inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    //printf("%d ", dInode.type);
    if (dInode.type == 0 && inodeRealUse[(int) inode]) {
      printf("%d\n", inode);
      printf("inode referred to in directory but marked free.\n");
      close(fsfd);
      exit(1);
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

  for(uint inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    //printf("%d ", dInode.type);
    if (dInode.type != 0 && !inodeRealUse[(int) inode]) {
      printf("%d\n", inode);
      printf("inode marked use but not found in a directory.\n");
      close(fsfd);
      exit(1);
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

int checkParentDir(){ //5
  uint inode;
  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    if (dInode.type == T_DIR) {
      for(int dBlock = 0; dBlock < NDIRECT; dBlock++) {
        // no map is okay
        if(dInode.addrs[dBlock] == 0)
          continue;
        if(!checkParentDirAtAddress(dInode.addrs[dBlock])){
          printf("parent directory mismatch.\n");
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
        if(indirectPointer[indirect]== 0)
          continue;
        if(!checkParentDirAtAddress(indirectPointer[indirect])){
          printf("parent directory mismatch.\n");
          close(fsfd);
          exit(1);
        }
      }
    }
  }
  return 0;
}

int checkDirFormat(){ //4
  uint inode;
  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    if (dInode.type == T_DIR) {
      //printf("E4 %d\n", findDir(dInode.addrs[0], "."));
      for(int dBlock = 0; dBlock < NDIRECT; dBlock++) {
        // no map is okay
        if(dInode.addrs[dBlock] == 0)
          continue;
        if(!findDir(dInode.addrs[dBlock], ".") || !findDir(dInode.addrs[dBlock], "..")){
          printf("directory not properly formatted.\n");
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
        if(indirectPointer[indirect]== 0)
          continue;
        if(!findDir(indirectPointer[indirect], ".") || !findDir(indirectPointer[indirect], "..")){
          printf("directory not properly formatted.\n");
          close(fsfd);
          exit(1);
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
  //printf("\n%d \n", address % (8 * BSIZE));
  return ((buf[address % (8 * BSIZE) / 8] >> (address % 8)) % 2);
}

int checkRootDir(){
  struct dinode dInode;
  rinode(1, &dInode);
  if(dInode.type != T_DIR){
    printf("%d\n", dInode.type);
    printf("root directory does not exist.\n");
    close(fsfd);
    exit(1);
  }
  return 0;
}

int checkBadInode(){ //1
  // check Error 1
  uint inode;
  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    //printf("%d ", dInode.type);
    if (dInode.type != 0 && dInode.type != T_FILE && dInode.type != T_DIR && dInode.type != T_DEV) {
      printf("bad inode.\n");
      close(fsfd);
      exit(1);
    }
  }
  return 0;
}

int checkBadAddressInInode(){//2
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
          printf("bad address in inode.\n");
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
          printf("bad address in inode.\n");
          close(fsfd);
          exit(1);
        }
      }
    }
  }
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
      if (!checkBmap(dInode.addrs[dBlock])){
        printf("address used by inode but marked free in bitmap.\n");
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
      if(!checkBmap(indirectPointer[indirect])){
        printf("%d %d \n", indirectPointer[indirect], checkBmap(indirectPointer[indirect]));
        printf("address used by inode but marked free in bitmap.\n");
        close(fsfd);
        exit(1);
      }
    }
  }
  return 0;
}

int checkUsedBitmap(){ //7
  uint inode;
  int inUsedBlock[nblocks];
  for(int i = 0; i < nblocks; i++){
    inUsedBlock[i] = 0;
    //printf("%d %d\n", i, checkBmap(i + nmeta));
  }

  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    if (dInode.type == 0)
      continue;
    for(int dBlock = 0; dBlock <= NDIRECT; dBlock++) {
      //printf("%d\n", dInode.addrs[dBlock] - nmeta);
      if (dInode.addrs[dBlock] == 0)
        continue;
      inUsedBlock[dInode.addrs[dBlock] - nmeta] = 1;
    }
    
    if(dInode.addrs[NDIRECT] == 0)
      continue;
    uchar buf[BSIZE];
    rsect(dInode.addrs[NDIRECT], buf);
    uint *indirectPointer = (uint*)buf;
    for(int indirect = 0; indirect < NINDIRECT; indirect++){
      if(indirectPointer[indirect] == 0)
        continue;
      inUsedBlock[indirectPointer[indirect] - nmeta] = 1;
    }
  }
  for(int i = 0; i < nblocks; i++){
    //printf("%d %d %d\n", i, inUsedBlock[i], checkBmap(i + nmeta));
    if(checkBmap(i + nmeta) != inUsedBlock[i]){
      printf("bitmap marks block in use but it is not in use.\n");
      close(fsfd);
      exit(1);
    }
  }
  return 0;
}

int checkAddressInUsedOnce(){ //8
  uint inode;
  int inUsedBlock[nblocks];
  for(int i = 0; i < nblocks; i++){
    inUsedBlock[i] = 0;
    //printf("%d %d\n", i, checkBmap(i + nmeta));
  }

  for(inode = 0; inode < sb.ninodes; inode++) {
    struct dinode dInode;
    rinode(inode, &dInode);
    if (dInode.type == 0)
      continue;
    for(int dBlock = 0; dBlock <= NDIRECT; dBlock++) {
      //printf("%d\n", dInode.addrs[dBlock] - nmeta);
      if (dInode.addrs[dBlock] == 0)
        continue;
      if(inUsedBlock[dInode.addrs[dBlock] - nmeta] == 1){
        printf("address used more than once.\n");
        close(fsfd);
        exit(1);
      }else{
        inUsedBlock[dInode.addrs[dBlock] - nmeta] = 1;
      }
    }
    
    if(dInode.addrs[NDIRECT] == 0)
      continue;
    uchar buf[BSIZE];
    rsect(dInode.addrs[NDIRECT], buf);
    uint *indirectPointer = (uint*)buf;
    for(int indirect = 0; indirect < NINDIRECT; indirect++){
      if(indirectPointer[indirect] == 0)
        continue;
      if(inUsedBlock[indirectPointer[indirect] - nmeta] == 1){
        printf("address used more than once.\n");
        close(fsfd);
        exit(1);
      }else{
        inUsedBlock[indirectPointer[indirect] - nmeta] = 1;
      }
    }
  }
  //for(int i = 0; i < nblocks; i++){
  //  //printf("%d %d %d\n", i, inUsedBlock[i], checkBmap(i + nmeta));
  //  if(checkBmap(i + nmeta) != inUsedBlock[i]){
  //    printf("bitmap marks block in use but it is not in use.\n");
  //    close(fsfd);
  //    exit(1);
  //  }
  //}
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

