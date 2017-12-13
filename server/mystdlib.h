#ifndef MY_STDLIB_H
#define MY_STDLIB_H


#define MEMORY_SIZE (1000* 1024)
#define PAGE_SIZE 128	//bytes
#define BLOCK_SIZE (MEMORY_SIZE / PAGE_SIZE)	//pages

#define INVALID_BIT 0x0080
#define VALID_BLOCKS 0x00
#define INVALID_BLOCKS(blk) (INVALID_BIT | (blk & 0x007f))
#define PAGE_LENGTH(blk) (blk & 0x007f)
#define IS_VALID(v) v>=0
#define IS_INVALID(v) v<0


#ifdef M3_DEBUG
//*****************************************************************************
//
// Map all debug print calls to UARTprintf in debug builds.
//
//*****************************************************************************
#ifdef M3_TEST
	extern void UARTprintf(const char *pcString, ...);
	#define DEBUG_PRINT UARTprintf
#else
	#define DEBUG_PRINT printf
#endif
#else
//*****************************************************************************
//
// Compile out all debug print calls in release builds.
//
//*****************************************************************************
#define DEBUG_PRINT while(0) ((int (*)(char *, ...))0)
#endif

#ifndef NULL
	#define NULL 0
#endif

void *testmy_malloc(unsigned int size);
void testmy_free(void *ptr);

#endif


