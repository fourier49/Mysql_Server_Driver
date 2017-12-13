#include <stdio.h>
#include <stdlib.h>

#include "mystdlib.h"


unsigned char memoryPound[BLOCK_SIZE][PAGE_SIZE];
char validPound[BLOCK_SIZE] = {0};
//bits   7   [6 - 0]
//mean valid [offset]
int restPound = BLOCK_SIZE;

void *testmy_malloc(unsigned int size)
{
	unsigned int i, n, nBlocks;

	printf("testmy_malloc \n");
	
	//*************************
	// Check Correctness
	//*************************
	if (size == 0) return NULL;
	
	if (size % PAGE_SIZE == 0) {//require memory size can be divided perfectly.
		nBlocks = size / PAGE_SIZE;
	}
	else {//Need one more page to fit the requiring memory size.
		nBlocks = (size / PAGE_SIZE) + 1;
	}
	if (nBlocks > restPound) return NULL;
	
	//*************************
	// Find free blocks
	//*************************
	n = 0;
	for (i = 0; i < BLOCK_SIZE && n < nBlocks; i++) {
		if (IS_VALID(validPound[i]))
			n++;
		else
			n = 0;
	}
	//There's no enough memory for this requiring.
	if (n != nBlocks)
		return NULL;
	
	i -= n;	//Calculate the base of blocks.
	n = i;	//restore the pointer for this request to n.
	
	//*************************
	// Invalidate and allocate blocks
	//*************************
	restPound -= nBlocks;
	//DEBUG_PRINT("ALLOCATE at %d blocks %d   , rest %d\n", n, nBlocks, restPound);
	//Invalidate all the blocks
	for (i = n; nBlocks > 0; i++, nBlocks--) {
		validPound[i] = INVALID_BLOCKS(nBlocks);
	}
	if (restPound < 0) {
		DEBUG_PRINT("ERROR: Memory leakage!!!\n");
	}
	printf("testmy_malloc end\n");
	
	return memoryPound[n];
}

void testmy_free(void *ptr)
{	
	unsigned int i, n, nBlocks;
	printf("testmy_free \n");
	//*************************
	// NULL pointer error
	//*************************
	if (ptr == NULL) return ;
	
	//*************************
	// Pointer is out of the heap range.
	//*************************
	if ((int)ptr < (int)memoryPound[0]) {
		DEBUG_PRINT("ERROR: Out of range\n");
		return ;
	}
	//*************************
	// Calculate the block number and length
	//*************************
	n = (int)ptr - (int)memoryPound[0];
	n = n / PAGE_SIZE;
	nBlocks = PAGE_LENGTH(validPound[n]);
	//*************************
	// The block is already validated.
	//*************************
	if (IS_VALID(validPound[n])) {
		DEBUG_PRINT("ERROR: This block is already validated\n");
		return ;
	}
	//*************************
	// The block size is out of the range of pound.
	//*************************
	if (n + nBlocks > BLOCK_SIZE) {
		DEBUG_PRINT("ERROR: Memory leakage!!!-Free > Memory Size\n");
		return ;
	}
	
	//*************************
	// Increase the pound counter
	//*************************
	restPound += nBlocks;
	//DEBUG_PRINT("FREE at %d blocks %d   , rest %d\n", n, nBlocks, restPound);

	//*************************
	// Invalidate blocks
	//*************************
	for (i = n; i < n + nBlocks; i++) {
		validPound[i] = VALID_BLOCKS;
	}
	printf("testmy_free end \n");
	
	return ;
}

