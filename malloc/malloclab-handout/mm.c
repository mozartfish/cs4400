/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * 
 * Pranav Rajan
 * Implements 4 basic strategies to quickly and efficiently allocate memory
 * 1) Explicit free list
 * 2) coalesce free blocks
 * 3) unmapping unused pages
 * 4) doubling chunk size 
 */
/*****************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "mm.h"
#include "memlib.h"

/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize() - 1)) & ~(mem_pagesize() - 1))

// This assumes you have a struct or typedef called "block_header" and "block_footer"
#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))

// Given a payload pointer, get the header or footer pointer
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - OVERHEAD)

// Given a payload pointer, get the next or previous payload pointer
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-OVERHEAD))
/*************************************************************************************/
// size_t macros
// Given a pointer to a header, get or set its value
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

// Combine a size and alloc bit
#define PACK(size, alloc) ((size) | (alloc))

// Given a header pointer, get the alloc or size
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_SIZE(p) (GET(p) & ~0xF)

// overall overhead for a page
#define PAGE_OVERHEAD 32
/**************************************************************************************/
/* HELPER FUNCTIONS */
static void extend(size_t new_size);

/****************************************************************************************************/

/* GLOBAL VARIABLES AND DATA STRUCTURES */
typedef size_t block_header;
typedef size_t block_footer;
// pointer to keep track of the first block payload
static void *first_bp;

// pointer to keep track of the subsequent block payloads
static void *bp;

/***************************************************************************************************/

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  first_bp = NULL;
  bp = NULL;
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  // variable to store the new size which is aligned to take care of the overhead
  int new_size = ALIGN(size + OVERHEAD);

  // call the extend function to get space for allocating stuff
  extend(new_size);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

static void extend(size_t new_size)
{
  // get a chunk of pages that satisfy the requested size
  size_t current_size = PAGE_ALIGN(new_size);

  // get a number p bytes that are equivalent to page_chunk_size
  void *p = mem_map(current_size);

  PUT(p, 16);                                                 // 8 bytes padding
  PUT(p + 8, PACK(OVERHEAD, 1));                              // Prologue Header
  PUT(p + 16, PACK(OVERHEAD, 1));                             // Prologue Footer
  PUT(p + 24, PACK(current_size - PAGE_OVERHEAD, 0));         // payload header
  first_bp = p + 32;                                          // payload pointer for the first block
  PUT(FTRP(first_bp), Pack(current_size - PAGE_OVERHEAD, 0)); // payload footer
  PUT(HDRP(NEXT_BLKP(first_bp)), PACK(0, 1));                 // epilogue header
}
