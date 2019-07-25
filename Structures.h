#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

/*struct stat
{
  int st_dev;
  int st_ino;
  int st_mode;
  int st_nlink;
  int st_uid;
  int st_gid;
  int st_rdev;
  int st_size;
  time_t st_atime;
  time_t st_mtime;
  time_t st_ctime;
  int st_blksize;
  int st_blocks;
};
*/

typedef struct inode
{
  int mode;
  int iNum;
  int size;
  int noChildren;
  int aTime;
  int dTime;
  int cTime;
  int noBlocks;
  int links; //Number of Links
  int blockSize;
  int type; //Directory or File
  char filename[20];
  char *path;
  char **blockPointers;
}inode;

typedef struct LLNode
{
  inode Inode;
  struct LLNode *next;  
}LLNode;


typedef struct SBlock
{
  int diskSize;
  int superBlockSize;
  int blockSize;
  int noBlocks;
  int noFreeBlocks;
  int noUsedBlocks;
  int inodeSize;
  int noInodes;
  int noFreeInodes;
  int noDataBlocks;
  int noFreeDataBlocks;
  int dataBitmapSize; //Data Bitmap size in Bytes
  int inodeBitmapSize; //Inode Bitmap size in Bytes
  int mountTime;
  int lastWriteTime;
  int lastReadTime;
  inode *firstInode;
}SBlock;


