#define FUSE_USE_VERSION 31
#define BLOCK_SIZE 4096
#define DISK_BLOCK 1024
#define INODE_SIZE 256
#define INODE_BLOCK 40
#define DATA_BLOCKS 979
#define SUPERBLOCK 1
#define INODE_BITMAP 2
#define DATA_BITMAP 2
#define SUPERSIZE BLOCK_SIZE

#define DIFF(A, B) (A>B)?(A-B):(B-A) 

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include "Structures.h"

SBlock *SuperBlock;
int *inodeBitmap;
int *dataBitmap;
LLNode *rootInode = NULL;
int inodeNum = 1;
int FUSE_file;
int **DirectoryChildren = NULL;
int rootChildren = 0;
int initFlag = 0;

static void retreiveFS()
{
	FILE *getData = fopen("../dataDump", "r");
	if(getData)
	{
		char *echo = "echo \"";
		char *makeDir = "mkdir ";
		char *putInput = "\">> ";

		char *path = "../The_File_System";
		char *command = (char*)malloc(sizeof(char)*(BLOCK_SIZE*10));
		char *str = (char*)malloc(sizeof(char)*(BLOCK_SIZE*10));
		char *fileName = (char*)malloc(sizeof(char)*256);
		

		while(!feof(getData))
		{
			memset(command, 0, strlen(command));
			fscanf(getData, "%[^\n]\n", str);
			if(str[0] == 'D')
			{
				strcpy(command, makeDir);
				strcat(command, path); 
				strcat(command, str+1);
				strcat(command, "\n");
				system(command);
				strcpy(command, "");
			}

			else if(str[0] == 'F')
			{
				char *token = strtok(str, ":");
				strcpy(fileName, token);
				strcpy(command, echo);
				token = strtok(NULL, ":");
				strcat(command, token);
				strcat(command, putInput);
				strcat(command, path);
				strcat(command, fileName+1);
				strcat(command, "\n");
				system(command);
				strcpy(command, "");
			}
		}

		free(command);
		free(str);
		free(fileName);
	}
	return;
}

static int findFreeDataNode(int iNo)
{
	int i=0;
	for(i=0; i<SuperBlock->noDataBlocks; i++)
	{
		if((dataBitmap[i]==0)&&(SuperBlock->noFreeDataBlocks>0))
		{
			SuperBlock->noFreeDataBlocks--;
			dataBitmap[i]=iNo;
			return i;
		}
	}
	return -1;

}

static void freeDataBlock(int iNo)
{
	int i=0;
	for(i=0; i<SuperBlock->noDataBlocks; i++)
	{
		if(dataBitmap[i]==iNo)
		{
			dataBitmap[i]=0;
			SuperBlock->noFreeDataBlocks++;
		}
	}
}

static void allocateInode(LLNode **First, int type, const char* pathOfFile, char *fname)
{

  inodeBitmap[inodeNum]=1;
  LLNode *x = (LLNode*)calloc(1, sizeof(LLNode));
  x->Inode.mode = 0;
  x->Inode.iNum = inodeNum;
  printf("iNum Alloc: %d\n", inodeNum);
  inodeNum++;
  x->Inode.noChildren = 0;
  x->Inode.aTime = time(NULL);
  x->Inode.cTime = time(NULL);
  x->Inode.dTime = time(NULL);
  x->Inode.noBlocks = 0;
  x->Inode.links = type;
  if(type==1)
  {
  	x->Inode.size = 0;
  	x->Inode.noBlocks = 1;
  	x->Inode.blockPointers = (char**)calloc(10, sizeof(char*));
  	findFreeDataNode(x->Inode.iNum);
  	x->Inode.blockPointers[0] = (char*)calloc(BLOCK_SIZE, sizeof(char));

  }
  else
  {
  	x->Inode.size = BLOCK_SIZE;
  	x->Inode.blockPointers = NULL;
  	x->Inode.noBlocks = 0;	
  }
  x->Inode.blockSize = BLOCK_SIZE;
  x->Inode.type=type;
  x->Inode.path = (char*)malloc(122*sizeof(char));
  strcpy(x->Inode.path, pathOfFile);
  strcpy(x->Inode.filename, fname);
  x->next = NULL;

  if(*First==NULL)
  {
  	printf("Allocating First Node\n");
  	*First = x;
  	return;
  }
  else
  {
  	LLNode *temp = *First;
  	while(temp->next!=NULL)
  	{
  		temp=temp->next;
  	}
  	temp->next = x;
  }

}

static int* makeBitmap(int size)
{
  int *Bmap = (int*)calloc(size, sizeof(int));
  return Bmap;
}

static int findFreeInode()
{
	int i=0;
	for(i=0; i<(SuperBlock->noFreeInodes); i++)
	{
		if((inodeBitmap[i]==0))
		{
			inodeBitmap[i]=inodeNum;
			SuperBlock->noFreeInodes--;	
			return i;
		}
	}
	return -1;
}

static LLNode* findInode(const char* path, int *iNo)
{
	LLNode* temp = rootInode;
	while(temp!=NULL)
	{
		if(strcmp(temp->Inode.path, path)==0)
		{
			*iNo = temp->Inode.iNum;
			return temp;
		}
		temp=temp->next;
	}
	return NULL;
}

static LLNode* findInodeByName(const char* fname, int *iNo)
{
	LLNode* temp = rootInode;
	while(temp!=NULL)
	{
		if(strcmp(temp->Inode.filename, fname)==0)
		{
			*iNo = temp->Inode.iNum;
			return temp;
		}
		temp=temp->next;
	}
	return NULL;
}

static LLNode* findInodeByNumber(int iNo)
{
	LLNode* temp = rootInode;
	printf("inodeNum: %d\n", inodeNum);
	while(temp!=NULL)
	{
		printf("%d : %d\n", temp->Inode.iNum, iNo);
		if(temp->Inode.iNum == iNo)
		{
			printf("FoundByNum\n");
			return temp;
		}
		temp=temp->next;
	}
	printf("!FoundByNum\n");
	return NULL;
}

static void deleteChild(int pNo, int index, int noChildren)
{
	int i=0; 
	for(i=index; (i+1)<noChildren; i++)
	{
		DirectoryChildren[pNo][i] = DirectoryChildren[pNo][i+1];
	}
}

static void removeFileInode(int iNo)
{
	LLNode *node = findInodeByNumber(iNo);
	LLNode *temp = rootInode, *prev=NULL;
	
	while(temp->Inode.iNum!=iNo)
	{
		prev=temp;
		temp=temp->next;
	}
	if(prev!=NULL)
	{
		prev->next = temp->next;
		temp->next = NULL;
	}
	else
	{
		rootInode=NULL;
	}
	
	int noBlocks = node->Inode.noBlocks, i;
	for(i=0;  i<noBlocks; i++)
	{
		freeDataBlock(node->Inode.iNum);
		free(node->Inode.blockPointers[i]);
	}
	free(node->Inode.blockPointers);
	free(node->Inode.path);
	free(node);
	
	for(i=0; i<(SuperBlock->noFreeInodes); i++)
	{
		if((inodeBitmap[i]==iNo))
		{
			inodeBitmap[i]=0;
			SuperBlock->noFreeInodes++;	
			return;
		}
	}
}

static void removeDirInode(int iNo)
{	
	LLNode *node = findInodeByNumber(iNo);
	LLNode *temp = rootInode, *prev=NULL;
	
	while(temp->Inode.iNum!=iNo)
	{
		prev=temp;
		temp=temp->next;
	}
	if(prev!=NULL)
	{
		prev->next = temp->next;
		temp->next = NULL;
	}
	else
	{
		rootInode=NULL;
	}
	
	free(node->Inode.path);
	free(node);
	
	int i;
	for(i=0; i<(SuperBlock->noFreeInodes); i++)
	{
		if((inodeBitmap[i]==iNo))
		{
			inodeBitmap[i]=0;
			SuperBlock->noFreeInodes++;	
			return;
		}
	}

}

static void makePersistent(LLNode* Root) 
{

	int i=0;
	FILE *diskfile = fopen("../dataDump", "w");
	LLNode* temp = Root;
	char *writeDump = (char*)calloc(10*BLOCK_SIZE+128, sizeof(char)); 
	char *marker = ":"; 
	char *newLine = "\n";
	char *File = "F";
	char *Dir = "D";
	while(temp!=NULL)
	{
		if(temp->Inode.type==1)
		{
			strcpy(writeDump, File);
		}
		else
		{
			strcpy(writeDump, Dir);
		}

		strcat(writeDump, temp->Inode.path);
		printf("%s\n", temp->Inode.path);
		if ((temp->Inode.type==1) && (temp->Inode.size>0)) 
		{
			strcat(writeDump, marker);
			for(i=0; i<temp->Inode.noBlocks; i++)
			{
				strcat(writeDump, temp->Inode.blockPointers[i]);
			}
		}
		strcat(writeDump, newLine);
		fwrite(writeDump, strlen(writeDump), 1, diskfile); 
		temp = temp->next;
	}
	free(writeDump); 
	fclose(diskfile);
	return;
}

static void* FUSE_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	(void)conn;
	(void)cfg;
	return conn;
}
static void FUSE_destroy(void *private_data)
{
	if(rootInode==NULL)
	{
		return;
	}
	else
	{
		makePersistent(rootInode);
		retreiveFS();
		return;
	}
}

static int FUSE_access(const char* path, int mask)
{

	if(strcmp(path, "/")==0)
	{
		return 0;
	}

	int iNo;
	LLNode *node = findInode(path, &iNo);
	if((sizeof(path)<=1)||(node==NULL))
	{
		return -ENOENT;
	}

	return 0;
}

static int FUSE_getattr(const char *path, struct stat *stbuf)
{
	if(sizeof(path)<2)
	{
		printf("Invalid Path: getattr\n");
		return -1;
	}

	int iNo=0;
	memset(stbuf, 0, sizeof(struct stat));
	
	if( strcmp( path, "/" ) == 0 )
	{
		int i = 0;
		LLNode *temp;
		stbuf->st_mode = S_IFDIR | 0666;
		stbuf->st_nlink = 2;
		for(i=0; i<rootChildren; i++)
		{
			temp = findInodeByNumber(DirectoryChildren[0][i]);
			if(temp->Inode.type==2)
			{
				stbuf->st_nlink++;
			}
		}
		stbuf->st_uid = 0;
		stbuf->st_gid = 0;
		stbuf->st_size = BLOCK_SIZE;
		stbuf->st_atime = time(NULL);
		stbuf->st_ctime = time(NULL);
		stbuf->st_mtime = time(NULL);
		stbuf->st_blksize = BLOCK_SIZE;
		stbuf->st_dev = 0;
		stbuf->st_rdev = 0;
		stbuf->st_ino = iNo;
		stbuf->st_blocks = 0;
	}
	
	else
	{ 
		LLNode *node = findInode(path, &iNo);
		if(node==NULL)
		{
			printf("Error: Invalid Path-%s\n", path);
			return -ENOENT;
		}
		stbuf->st_ino = node->Inode.iNum;
		if(node->Inode.type==2)
		{
			stbuf->st_mode = S_IFDIR | 0664;
		}
		if(node->Inode.type==1)
		{
			stbuf->st_mode = S_IFREG | 0664;
		}
		stbuf->st_nlink = node->Inode.links;
		stbuf->st_uid = node->Inode.iNum;
		stbuf->st_gid = 0;
		stbuf->st_size = node->Inode.size;
		stbuf->st_atime = node->Inode.aTime;
		stbuf->st_ctime = node->Inode.cTime;
		stbuf->st_mtime = node->Inode.dTime;
		stbuf->st_blksize = BLOCK_SIZE;
		stbuf->st_dev = 0;
		stbuf->st_rdev = 0;
		stbuf->st_blocks = node->Inode.noBlocks;

	}
	return 0;
}

static int FUSE_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	filler(buffer, ".", NULL, 0); 
	filler(buffer, "..", NULL, 0);
	LLNode *node, *child;
	int iNo, pNo;
	if ( strcmp( path, "/" ) == 0 ) 
	{
		if(rootInode!=NULL)
		{	
			printf("%s\n",rootInode->Inode.path );
			for(int i = 0; i<rootChildren; i++)
		  	{
		  		iNo = DirectoryChildren[0][i];
		  		child = findInodeByNumber(iNo);
		  		printf("%d\n", iNo);
		  		filler(buffer, child->Inode.filename, NULL, 0);
		  	}

		}
		return 0;
	}
	
	
  	
  	node=findInode(path, &pNo);
  	if(node ==NULL)
  	{
  		return -ENOENT;
  	}

  	for(int i = 0; i<node->Inode.noChildren; i++)
  	{
  		iNo = DirectoryChildren[pNo][i];
  		child = findInodeByNumber(iNo);
  		printf("%d\n", iNo);
  		filler(buffer, child->Inode.filename, NULL, 0);
  	}
  	node->Inode.aTime = time(NULL);
	return 0;
}

static int FUSE_open(const char *path, struct fuse_file_info *fi)
{
  	if(strcmp(path, "/")==0)
  	{
  		return 1;
  	}
  	int iNo;
  	LLNode *node;
  	node=findInode(path, &iNo);
  	if(node==NULL)
  	{
  		return -ENOENT;
  	}
  	return 0;
}

static int FUSE_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("READ %d\n", strlen(buf));
  	int iNo, i;
  	LLNode *node;
  	node = findInode(path, &iNo);
  	(void) fi;
  	
  	if(node == NULL)
  	{
  		return -ENOENT;
  	}

  	if(node->Inode.type==2)
  	{
  		printf("Cannot Read: Please Specify File, Not Directory\n");
  		return -1;
  	}

  	node->Inode.aTime = time(NULL);
  	if(offset==0)
  	{
  		printf("helloRead\n");
  		if(size > node->Inode.size)
  		{
  			size = node->Inode.size;
  		}
	  	int readSize = size;
	  	for(i=0; (i<node->Inode.noBlocks) ; i++)
	  	{
	  		if(readSize>BLOCK_SIZE)
	  		{
	  			//memcpy(buf, node->Inode.blockPointers[i], BLOCK_SIZE);
	  			if(i==0)
	  			{
	  				strcpy(buf, node->Inode.blockPointers[i]);
	  				continue;
	  			}
	  			strcat(buf, node->Inode.blockPointers[i]);
	  			readSize -= BLOCK_SIZE;
	  		}
	  		else if(readSize > 0)
	  		{
	  			if(i==0)
	  			{
	  				strcpy(buf, node->Inode.blockPointers[i]);
	  				continue;
	  			}

	  			strcat(buf, node->Inode.blockPointers[i]);
	  			break;
	  		}
	  	}
	}
	else
	{
		printf("helloOffset: %d\n", offset);
		int newOffset = offset, j;
		if(offset > BLOCK_SIZE)
		{
			i = 0;
			while(BLOCK_SIZE*i < offset)
			{
				i++;
				newOffset -= BLOCK_SIZE;
			}

			i--;
			if(size + offset > node->Inode.size)
			{
				size = node->Inode.size	- offset;
				if(size > BLOCK_SIZE)
				{
					int readSize = size;
					memcpy(buf, node->Inode.blockPointers[i] + newOffset, BLOCK_SIZE - newOffset);
					for(int j = i+1; j < node->Inode.noBlocks; j++)
					{
				  		if(readSize>BLOCK_SIZE)
				  		{
				  			memcpy(buf, node->Inode.blockPointers[i], BLOCK_SIZE);
				  			readSize -= BLOCK_SIZE;
				  		}
				  		else
				  		{
				  			memcpy(buf, node->Inode.blockPointers[i], readSize);
				  			break;
				  		}

					}
				}
				else
				{
					memcpy(buf, node->Inode.blockPointers[i], size);
				}
			}
		}

		else 
		{
			int readSize = size;
			memcpy(buf, node->Inode.blockPointers[0] + offset, DIFF(readSize, BLOCK_SIZE));
			readSize -= BLOCK_SIZE;
		  	for(i=1; i<node->Inode.noBlocks; i++)
		  	{
		  		if(readSize>BLOCK_SIZE)
		  		{
		  			memcpy(buf, node->Inode.blockPointers[i], BLOCK_SIZE);
		  			readSize -= BLOCK_SIZE;
		  		}
		  		else if(readSize > 0)
		  		{
		  			memcpy(buf, node->Inode.blockPointers[i], readSize);
		  			break;
		  		}
		  	}

		}
	}
  	return size;

}

static int FUSE_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("WRITE %d\n", size);
	(void)fi;
	int iNo;
	LLNode *node;
  	node = findInode(path, &iNo);
  	if(node == NULL)
  	{
  		return -ENOENT;
  	}

  	if(node->Inode.type==2)
  	{
  		printf("Cannot Write: Please Specify File, Not Directory\n");
  		return -1;
  	}

  	int fileSize = node->Inode.size;
  	printf("SIZE - FILESIZE: %d\n", size - fileSize);

  	if((fileSize>0)&&(size == BLOCK_SIZE)&&(fileSize % BLOCK_SIZE) == 0)
  	{
  		node->Inode.aTime = time(NULL);
  		node->Inode.cTime = time(NULL);
  		findFreeDataNode(node->Inode.iNum);
  		node->Inode.blockPointers[node->Inode.noBlocks] = (char*)calloc(BLOCK_SIZE, sizeof(char));
  		strncpy((node->Inode.blockPointers[node->Inode.noBlocks]), buf, BLOCK_SIZE);
  		node->Inode.size += size;
  		node->Inode.noBlocks++;
  		return size;
  	}
  	else if(BLOCK_SIZE - (fileSize % BLOCK_SIZE) == size)
  	{
  		node->Inode.aTime = time(NULL);
  		node->Inode.cTime = time(NULL);
  		strncpy((node->Inode.blockPointers[node->Inode.noBlocks-1]+(fileSize%BLOCK_SIZE)), buf, size);
  		node->Inode.blockPointers[node->Inode.noBlocks] = (char*)calloc(BLOCK_SIZE, sizeof(char));
  		node->Inode.noBlocks++;
  		node->Inode.size += size;
  		return size;
  	}
  	else if(BLOCK_SIZE - (fileSize % BLOCK_SIZE) > size)
  	{
  		node->Inode.aTime = time(NULL);
  		node->Inode.cTime = time(NULL);
  		strncpy((node->Inode.blockPointers[node->Inode.noBlocks-1]+(fileSize%BLOCK_SIZE)), buf, size);
  		node->Inode.size += size;
  		return size;
  	}
  	
  	else if(BLOCK_SIZE - (fileSize % BLOCK_SIZE) < size)
  	{
  		int newSize = size - (BLOCK_SIZE - (fileSize % BLOCK_SIZE));

  		node->Inode.aTime = time(NULL);
  		node->Inode.cTime = time(NULL);
  		findFreeDataNode(node->Inode.iNum);
  		strncpy((node->Inode.blockPointers[node->Inode.noBlocks-1]), buf, newSize);
  		node->Inode.blockPointers[node->Inode.noBlocks] = (char*)calloc(BLOCK_SIZE, sizeof(char));
  		strncpy((node->Inode.blockPointers[node->Inode.noBlocks]), buf+newSize, size - newSize);
  		node->Inode.noBlocks++;
  		printf("%d\n", node->Inode.noBlocks);
  		node->Inode.size += size;
  		return size;
  	}
  	return -1;
}

static int FUSE_rename(const char *from, const char *to, unsigned int flags)
{
  	int iNo;
	LLNode *node;
  	node = findInode(from, &iNo);
  	if(node == NULL)
  	{
  		return -ENOENT;
  	}
  	
}

static int FUSE_truncate(const char *path, off_t size, struct fuse_file_info *fi)
{
  	return 0;
}

static int FUSE_unlink(const char* path)
{
	int iNo;
	LLNode *temp;
	temp = findInode(path, &iNo);

	if(temp==NULL)
	{
		printf("Cannot Remove File: Invalid Path Name\n");
		return -1;
	}

	if(temp->Inode.type==2)
  	{
  		printf("Cannot Remove File: Please Specify File, Not Directory\n");
  		return -1;
  	}

	int pNo;
	char *token=NULL, *prevToken=NULL, *parentToken = NULL;
	const char* s = "/";
	char *tokenPath = (char*)malloc(strlen(path)+1);
	strcpy(tokenPath, path);
   	token = strtok(tokenPath, s);
   	while(token != NULL) 
   	{
		printf("%s\n", token);
		parentToken = prevToken;
		prevToken = token;
    	token = strtok(NULL, s);
   	}
   	if(prevToken!=NULL)
   	{
   		printf("%s\n", prevToken);
   		if(parentToken==NULL)
   		{
   			/*DirectoryChildren[0][rootChildren++] = inodeNum-1;*/
   			int i=0;
   			for(i=0; i<rootChildren; i++)
   			{
   				if(DirectoryChildren[0][i]==iNo)
   				{
   					deleteChild(0, i, rootChildren);
   					removeFileInode(iNo);
   					rootChildren--;
   					break;
   				}
   			}
   		}
   	}

   	LLNode* pNode=NULL;
   	if((parentToken!=NULL)&&(strcmp(parentToken, "/")!=0))
   	{
   		pNode = findInodeByName(parentToken, &pNo);
   	}

   	free(tokenPath);

   	if(pNode!=NULL)
   	{
   		/*DirectoryChildren[pNo][(pNode->Inode.noChildren)++] = inodeNum-1;*/
   		int i=0;
   		for(i=0; i<pNode->Inode.noChildren; i++)
   		{
   			if(DirectoryChildren[pNo][i]==iNo)
   			{
   				deleteChild(pNo, i, pNode->Inode.noChildren);
   				removeFileInode(iNo);
   				pNode->Inode.noChildren--;
   				break;
   			}
   		}
   	}
   	return 0;

}

static int FUSE_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int iNo;
	LLNode *temp;
	(void)fi;
	temp = findInode(path, &iNo);
	if(temp!=NULL)
	{
		printf("Cannot Create: File Already Exists\n");
		return -1;
	}

	iNo = findFreeInode();
	if(iNo==-1)
	{
		printf("Cannot Create: Storage Size Exceeded\n");
		return -1;
	}

	int pNo;
	char *token=NULL, *prevToken=NULL, *parentToken = NULL;
	const char* s = "/";
	char *tokenPath = (char*)malloc(strlen(path)+1);
	strcpy(tokenPath, path);
   	token = strtok(tokenPath, s);
   	while(token != NULL) 
   	{
		printf("%s\n", token);
		parentToken = prevToken;
		prevToken = token;
    	token = strtok(NULL, s);
   	}
   	if(prevToken!=NULL)
   	{
   		printf("%s\n", prevToken);
   		allocateInode(&rootInode, 1, path, prevToken);
   		if(parentToken==NULL)
   		{
   			DirectoryChildren[0][rootChildren++] = inodeNum-1;
   		}
   	}

   	LLNode* pNode=NULL;
   	if((parentToken!=NULL)&&(strcmp(parentToken, "/")!=0))
   	{
   		pNode = findInodeByName(parentToken, &pNo);
   	}

   	free(tokenPath);
   	if(pNode!=NULL)
   	{
   		DirectoryChildren[pNo][(pNode->Inode.noChildren)++] = inodeNum-1;
   	}

   	return 0;
}	

static int FUSE_mkdir(const char *path, mode_t mode)
{
	int iNo;
	int index = findFreeInode();
	printf("%d\n", inodeNum);
	if(index==-1)
	{
		printf("Cannot Make Directory: Storage Size Exceeded\n");
		return 0;
	}
	
	if((strcmp(path, "")==0)||(strcmp(path, " ")==0))
	{
		printf("Cannot Make Directory: Invalid Path Name\n");
		return -1;
	}
	LLNode *node = findInode(path, &iNo);
	if(node!=NULL)
	{
		
		printf("Cannot Make Directory: Already Exists\n");
		return -1;
	}		
	

	int pNo;
	char *token=NULL, *prevToken=NULL, *parentToken = NULL;
	const char* s = "/";
	char *tokenPath = (char*)malloc(strlen(path)+1);
	strcpy(tokenPath, path);
   	token = strtok(tokenPath, s);
   	while(token != NULL) 
   	{
		printf("%s\n", token);
		parentToken = prevToken;
		prevToken = token;
    	token = strtok(NULL, s);
   	}
   	if(prevToken!=NULL)
   	{
   		printf("%s\n", prevToken);
   		allocateInode(&rootInode, 2, path, prevToken);
   		if(parentToken==NULL)
   		{
   			DirectoryChildren[0][rootChildren++] = inodeNum-1;
   		}
   	}

   	LLNode* pNode=NULL;
   	if((parentToken!=NULL)&&(strcmp(parentToken, "/")!=0))
   	{
   		pNode = findInodeByName(parentToken, &pNo);
   	}

   	free(tokenPath);
   	if(pNode!=NULL)
   	{
   		DirectoryChildren[pNo][(pNode->Inode.noChildren)++] = inodeNum-1;
   		pNode->Inode.links++;
   	}

	return 0;
}

static int FUSE_rmdir(const char* path) /*Remove Directory Only If Empty*/
{
	int iNo;
	LLNode *temp;
	temp = findInode(path, &iNo);
	if(temp==NULL)
	{
		printf("Cannot Remove Directory: Invalid Path Name\n");
		return -1;
	}

	if(temp->Inode.type==1)
	{
		printf("Cannot Remove Directory: Please Specify Directory, Not File\n");
		return -1;	
	}

	if(temp->Inode.noChildren>0)
	{
		printf("Cannot Remove Directory: Directory Not Empty\n");
		return -ENOTEMPTY;
	}

	int pNo;
	char *token=NULL, *prevToken=NULL, *parentToken = NULL;
	const char* s = "/";
	char *tokenPath = (char*)malloc(strlen(path)+1);
	strcpy(tokenPath, path);
   	token = strtok(tokenPath, s);
   	while(token != NULL) 
   	{
		printf("%s\n", token);
		parentToken = prevToken;
		prevToken = token;
    	token = strtok(NULL, s);
   	}

   	if(prevToken!=NULL)
   	{
   		printf("%s\n", prevToken);
   		if(parentToken==NULL)
   		{
   			int i=0;
   			for(i=0; i<rootChildren; i++)
   			{
   				if(DirectoryChildren[0][i]==iNo)
   				{
   					deleteChild(0, i, rootChildren);
   					removeDirInode(iNo);
   					rootChildren--;
   					break;
   				}
   			}
   		}
   	}

   	LLNode* pNode=NULL;
   	if((parentToken!=NULL)&&(strcmp(parentToken, "/")!=0))
   	{
   		pNode = findInodeByName(parentToken, &pNo);
   	}

   	free(tokenPath);

   	if(pNode!=NULL)
   	{
   		/*DirectoryChildren[pNo][(pNode->Inode.noChildren)++] = inodeNum-1;*/
   		int i=0;
   		for(i=0; i<pNode->Inode.noChildren; i++)
   		{
   			if(DirectoryChildren[pNo][i]==iNo)
   			{
   				deleteChild(pNo, i, pNode->Inode.noChildren);
   				removeDirInode(iNo);
   				pNode->Inode.noChildren--;
   				pNode->Inode.links--;
   				break;
   			}
   		}

   	}
   	return 0;
}

static struct fuse_operations operations = {
	.getattr 	= FUSE_getattr,
	.readdir	= FUSE_readdir,
	.open 		= FUSE_open,
	.read 		= FUSE_read,
	.create 	= FUSE_create, 
	.mkdir 		= FUSE_mkdir,
	.access 	= FUSE_access,
	.rename 	= FUSE_rename,
	.truncate 	= FUSE_truncate,
	.write 		= FUSE_write,
	.rmdir 		= FUSE_rmdir,
	.unlink 	= FUSE_unlink,
	.destroy 	= FUSE_destroy,
	.init 		= FUSE_init,	
};



static void getFSFromStorage()
{
	FILE *getData = fopen("../dataDump", "r");
	if(getData)
	{
		printf("Hmmmm, Lets go!\n");
		char *str = (char*)malloc(sizeof(char)*(BLOCK_SIZE*10));
		char *fileName = (char*)malloc(sizeof(char)*256);
		

		while(!feof(getData))
		{
			fscanf(getData, "%[^\n]\n", str);
			if(str[0] == 'D')
			{
				FUSE_mkdir(str+1, 0);
			}

			else if(str[0] == 'F')
			{
				char *token = strtok(str, ":");
				strcpy(fileName, token+1);
				token = strtok(NULL, ":");
				struct fuse_file_info *X  = (struct fuse_file_info*)calloc(1, sizeof(struct fuse_file_info));
				FUSE_create(fileName, 0, X);
				if(strlen(token)<BLOCK_SIZE)
				{
					FUSE_write(fileName, token, strlen(token)+1, 0, X);
				}
				else
				{
					int i = 0;
					for(i=0; i<strlen(token); i+=BLOCK_SIZE)
					{
						FUSE_write(fileName, token+i, BLOCK_SIZE, 0, X);
					}

				}
			}
		}
		free(str);
		free(fileName);
	}

	return;
}
int main(int argc, char *argv[])
{
  	SuperBlock = (SBlock*)calloc(1, sizeof(SBlock));
  	SuperBlock->diskSize = DISK_BLOCK*BLOCK_SIZE;
  	SuperBlock->blockSize = BLOCK_SIZE;
  	SuperBlock->noBlocks = DISK_BLOCK;
  	SuperBlock->noFreeBlocks = DISK_BLOCK - (SUPERSIZE + INODE_BITMAP + DATA_BITMAP);
  	SuperBlock->noUsedBlocks = (SUPERSIZE + INODE_BITMAP + DATA_BITMAP);
  	SuperBlock->inodeSize = INODE_SIZE;
  	SuperBlock->noInodes = (INODE_BLOCK*BLOCK_SIZE)/INODE_SIZE;
  	SuperBlock->noFreeInodes = SuperBlock->noInodes;
  	SuperBlock->noDataBlocks = DATA_BLOCKS;
  	SuperBlock->noFreeDataBlocks = DATA_BLOCKS;
  	SuperBlock->inodeBitmapSize = INODE_BITMAP * BLOCK_SIZE;
  	SuperBlock->dataBitmapSize = DATA_BITMAP * BLOCK_SIZE;
  	SuperBlock->mountTime = time(NULL);
  	SuperBlock->lastWriteTime  = 0;
  	SuperBlock->lastReadTime = 0;
  	SuperBlock->superBlockSize = SUPERBLOCK * BLOCK_SIZE;
  	  

	char *clearFS = " rm -rd ../The_File_System/*";
	system(clearFS);

  	inodeBitmap = makeBitmap(2048);
  	dataBitmap = makeBitmap(2048);

  	DirectoryChildren = (int**)calloc(SuperBlock->noInodes, sizeof(int*));
  	for(int i=0; i<SuperBlock->noInodes; i++)
  	{
  		DirectoryChildren[i] = (int*)calloc(10, sizeof(int));
  	}

  	getFSFromStorage();

 	return fuse_main(argc, argv, &operations, NULL);
}

