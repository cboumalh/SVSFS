/*
 * SVSFS - a close copy of D. Thain's SimpleFS.
 * PJF 4/2023
 * original comments below
 * Implementation of SimpleFS.
 * Make your changes here.
*/

#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>

extern struct disk *thedisk;

#define FS_MAGIC           0x34341023
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 3
#define POINTERS_PER_BLOCK 1024

struct fs_superblock {
	uint32_t magic;
	uint32_t nblocks;
	uint32_t ninodeblocks;
	uint32_t ninodes;
};

struct fs_inode {
	uint32_t isvalid;
	uint32_t size;
	int64_t ctime;
	uint32_t direct[POINTERS_PER_INODE];
	uint32_t indirect;
};

union fs_block {
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK];
	int pointers[POINTERS_PER_BLOCK];
	unsigned char data[BLOCK_SIZE];
};

int mounted = (1==0);
unsigned char *freeblock = NULL;

int fs_format()
{
	// Determine NINODEBLOCKS
	int nblocks = disk_nblocks(thedisk);
	int ninodes = (int) ceil(nblocks / 10.0);
	
	// Declare Block B
	union fs_block block;

	// Fill in Superblock in B
	block.super.magic = FS_MAGIC;
	block.super.nblocks = nblocks;
	block.super.ninodeblocks = ninodes;
	block.super.ninodes = ninodes * INODES_PER_BLOCK;
	disk_write(thedisk,0,block.data);

	// Fill in inode Blocks in B
	for (int i=0; i<ninodes; i++)
	{
		for (int j=0; j<INODES_PER_BLOCK; j++)
		{
			block.inode[j].isvalid = 0;
			block.inode[j].size = 0;
			block.inode[j].ctime = 0;
			for (int k=0; k<POINTERS_PER_INODE; k++)
			{
				block.inode[j].direct[k] = 0;
			}
			block.inode[j].indirect = 0;
		}
		disk_write(thedisk,i+1,block.data);
	}

	return 1;
}

void fs_debug()
{
	// read and print super block
	union fs_block block;
	disk_read(thedisk,0,block.data);

	printf("superblock:\n");
	printf("    %d blocks\n",block.super.nblocks);
	printf("    %d inode blocks\n",block.super.ninodeblocks);
	printf("    %d inodes\n",block.super.ninodes);

	// loop through inodes
	for (int i = 0; i < block.super.ninodes; i++) {

		// read inode block
		disk_read(thedisk,i+1,block.data);

		// loop through inodes in block
		for (int j=0; j < INODES_PER_BLOCK; j++) {

			// skip invalid inodes
			if (block.inode[j].isvalid == 0)
				continue;


			// print inode info
			printf("inode %d:\n",j+1);
			printf("    size: %d bytes\n",block.inode[j].size);
			printf("    direct blocks:");


			// loop through direct pointers
			for (int k=0; k < POINTERS_PER_INODE; k++) {
				// print direct pointers
				if (block.inode[j].direct[k] != 0)
					printf(" %d",block.inode[j].direct[k]);
			}
			printf("\n");

			// print indirect pointer
			if (block.inode[j].indirect != 0)
				printf(" %d\n",block.inode[j].indirect);
		}
	}
}

int fs_mount()
{
	// Mount Filesystem
	if (mounted == (1==1)) {
		
		return 0;
	}
	
	mounted = (1==1);

	// Load Super Block
	union fs_block block;
	disk_read(thedisk,0,block.data);

	// 

	return 0;
}

int fs_create()
{
	return 0;
}

int fs_delete( int inumber )
{
	return 0;
}

int fs_getsize( int inumber )
{
	return 0;
}

int fs_read( int inumber, unsigned char *data, int length, int offset )
{
	return 0;
}

int fs_write( int inumber, const unsigned char *data, int length, int offset )
{
	return 0;
}
