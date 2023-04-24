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
#include <time.h>
#include <stdlib.h>


extern struct disk *thedisk;

#define FS_MAGIC           0x34341023
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 3
#define POINTERS_PER_BLOCK 1024
int mounted = (1==0);
unsigned char *freeblock = NULL;
#define MIN(a,b) ((a)<(b)?(a):(b))
#define DEBUG 1
unsigned int nbrfreeblks;

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

void printfree() {
        for(int i=0;i<disk_nblocks(thedisk);i++) {
                printf("%02X",freeblock[i]);
        }
}

// set the bit indicating that block b is free.
void markfree(int b) {
        int ix = b/8; // 8 bits per byte; this is the index of the byte to modify
        unsigned char mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
        // OR in the bit that corresponds to the block we're talking about.
        freeblock[ix] |= mask[b%8];
}

// set the bit indicating that block b is used.
void markused(int b) {
        int ix = b/8; // 8 bits per byte; this is the index of the byte to modify
        unsigned char mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
        // AND the byte with the inverse of the bitmask to force the desired bit to 0
        freeblock[ix] &= ~mask[b%8];
}

// check to see if block b is free
int isfree(int b) {
        int ix = b/8; // 8 bits per byte; this is the byte number
        unsigned char mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
        // if bit is set, return nonzero; else return zero
        return ((freeblock[ix] & mask[b%8]) != 0);
}

int getfreeblock() {
        //printf("Searching %d blocks for a free block\n",disk_nblocks(thedisk));
        for(int i=0; i<disk_nblocks(thedisk);i++)
                if (isfree(i)) {
                        //printf("found free block at %d\n",i);
                        return i;
                }
        printf("No free blocks found\n");
        return -1;
}

// return number of free blocks
// This also expects the superblock and inode blocks to be marked unavailable
unsigned int nfreeblocks() {
        unsigned int n=0;
        for(int i=0;i<disk_nblocks(thedisk);i++)
                if (isfree(i)) n++;
        return n;
}

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
	struct fs_superblock superblock = block.super;

	printf("superblock:\n");
	printf("    %d blocks\n",superblock.nblocks);
	printf("    %d inode blocks\n",superblock.ninodeblocks);
	printf("    %d inodes\n",superblock.ninodes);

	// loop through inodes
	for (int i = 0; i < superblock.ninodeblocks; i++) {

		// read inode block
		disk_read(thedisk,i+1,block.data);
		
		// loop through inodes in block
		for (int j=0; j < INODES_PER_BLOCK; j++) {
			// skip invalid inodes
			if (block.inode[j].isvalid == 0)
				continue;


			// print inode info
			printf("inode %d:\n",j);
			printf("    valid: YES\n");
			printf("    size: %d bytes\n",block.inode[j].size);
			printf("    created: %s",ctime(&block.inode[j].ctime));
			printf("    direct blocks:");


			// loop through direct pointers
			for (int k=0; k < POINTERS_PER_INODE; k++) {
				// print direct pointers
				if (block.inode[j].direct[k] != 0)
					printf(" %d",block.inode[j].direct[k]);
			}
			printf("\n");

			// print indirect pointer
			if (block.inode[j].indirect != 0) {
				printf("    indirect blocks (in block %d): ",block.inode[j].indirect);
				
				// read indirect block
				union fs_block indirectblock;
				disk_read(thedisk,block.inode[j].indirect,indirectblock.data);

				for (int l=0; l < POINTERS_PER_BLOCK; l++) {
					// print indirect pointers
					if (indirectblock.pointers[l] != 0)
						printf(" %d",indirectblock.pointers[l]);
				}
				printf("\n");
			}
		}
	}
}

int fs_mount()
{
	// Mount Filesystem
	if (mounted == (1==1)) {
		printf("Already mounted\n");
		return 0;
	}
	
	mounted = (1==1);
	

	// Load Super Block
	union fs_block block;
	disk_read(thedisk,0,block.data);

	if(block.super.magic != FS_MAGIC){
		printf("Magic Number Incorrect\n");
		return 0;
	}

	if(block.super.ninodeblocks == 0 || block.super.nblocks == 0){
		printf("No blocks or inode blocks\n");
		return 0;
	}

	if (freeblock) free(freeblock);
	unsigned int nb = disk_nblocks(thedisk);
	nbrfreeblks = nb;
	unsigned int nfbb = nb*sizeof(unsigned char)/8 + ((nb%8) != 0) ? 1 : 0;
	freeblock = (unsigned char *)malloc(nfbb);
    if (freeblock == NULL) { perror("malloc failed"); return 0; }

	// initialize free block bitmap
	for(int i = 0; i < nb; i++)
		markfree(i);

	// mark super block and inode blocks as used
	markused(0);
	nbrfreeblks--;
	for(int i = 0; i < block.super.ninodeblocks; i++){
		markused(i + 1);
		nbrfreeblks--;
	}
		
	// mark used blocks
	for(int i = 0; i < block.super.ninodes; i++) {
		disk_read(thedisk,i+1,block.data);

		// loop through inodes in block
		for(int j = 0; j < INODES_PER_BLOCK; j++) {
			if(block.inode[j].isvalid == 0) continue;

			// mark direct pointers
			for(int k = 0; k < POINTERS_PER_INODE; k++) {
				if(block.inode[j].direct[k] == 0) continue;
				markused(block.inode[j].direct[k]);
				nbrfreeblks--;
			}

			// check indirect block
			if(block.inode[j].indirect == 0) continue;
			
			markused(block.inode[j].indirect);
			nbrfreeblks--;
			
			// mark indirect pointers
			disk_read(thedisk,block.inode[j].indirect,block.data);
			for(int k = 0; k < POINTERS_PER_BLOCK; k++) {
				if(block.pointers[k] == 0) continue;
				markused(block.pointers[k]);
				nbrfreeblks--;
			}
		}
	}


	mounted = (1==1);
	
	return 1;
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
