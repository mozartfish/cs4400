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
#define PAGE_OVERHEAD 48 // prolog header + prolog footer + epilog header + padding + chunk pointers
/**************************************************************************************/
/* HELPER FUNCTIONS */
static void extend(size_t new_size);
static void set_allocated(void *bp, size_t size);
static int heap_checker(void *bp);
static void *coalesce(void *bp);
static void add_page_chunk(void *memory);

/****************************************************************************************************/

/* GLOBAL VARIABLES AND DATA STRUCTURES */
typedef size_t block_header;
typedef size_t block_footer;

typedef struct page_chunk
{
  struct page_chunk *next;
  struct page_chunk *prev;
} page_chunk;

// pointer to keep track of the first block payload
static void *first_bp;

// pointer to keep track of the subsequent block payloads
static void *bp;

// global pointer to the first page chunk
static page_chunk *first_page_chunk = NULL;

/***************************************************************************************************/

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  first_bp = NULL;
  bp = NULL;
  first_page_chunk = NULL;
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

  // check if there is a page chunk available
  if (first_page_chunk == NULL)
  {
    extend(new_size);
  }

  while (1)
  {
    // set bp pointer
    bp = first_bp;

    while (GET_SIZE(HDRP(bp)) != 0)
    {
      if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp))) >= new_size)
      {
        set_allocated(bp, new_size);
        //heap_checker(first_bp);
        return bp;
      }
      else
      {
        bp = NEXT_BLKP(bp);
      }
    }

    // if we reach an epilogue check if there is another page chunk
    if (first_page_chunk->next == NULL)
    {
      extend(new_size);
    }
    else
    {
      bp = sizeof(page_chunk) + OVERHEAD + sizeof(block_header);
    }
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  size_t size = GET_SIZE(HDRP(ptr));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));

  // call the coalesce function
  coalesce(ptr);
}

static void extend(size_t new_size)
{
  // get a chunk of pages that satisfy the requested size
  size_t current_size = PAGE_ALIGN(new_size);

  // get a number p bytes that are equivalent to page_chunk_size
  void *p = mem_map(current_size);

  // printf("The current size returned is: %u", sizeof(p));

  // add a page chunk to the linked list
  add_page_chunk(p);

  p += sizeof(page_chunk);                            // move the p pointer past the first 16 bytes since that's assigned for the pages
  PUT(p, 0);                                          // padding of 8 bytes
  PUT(p + 8, PACK(OVERHEAD, 1));                      // PROLOGUE Header;
  PUT(p + 16, PACK(OVERHEAD, 1));                     // PROLOGUE FOOTER;
  PUT(p + 24, PACK(current_size - PAGE_OVERHEAD, 0)); // Payload Header
  first_bp = p + 32;                                 // Payload memory
  PUT(FTRP(first_bp), PACK(current_size - PAGE_OVERHEAD, 0)); // Payload Footer
  PUT(FTRP(first_bp) + 8, PACK(0, 1)); // EPILOGUE Header
  PUT(HDRP(NEXT_BLKP(first_bp)), FTRP(first_bp) + 8); // have the next block pointer be the epilogue

  // PUT(HDRP(NEXT_BLKP(first_bp)), PACK(0, 1));                 // EPILOGUE Header

  // PUT(p, 0);                                                  // 8 bytes padding
  // PUT(p + 8, PACK(OVERHEAD, 1));                              // Prologue Header
  // PUT(p + 16, PACK(OVERHEAD, 1));                             // Prologue Footer
  // PUT(p + 24, PACK(current_size - PAGE_OVERHEAD, 0));         // payload header
  // first_bp = p + 32;                                          // payload pointer for the first block
  // PUT(FTRP(first_bp), PACK(current_size - PAGE_OVERHEAD, 0)); // payload footer
  // PUT(HDRP(NEXT_BLKP(first_bp)), PACK(0, 1));                 // epilogue header
}

static int heap_checker(void *bp)
{
  void *p = NULL;

  // set bp pointer
  p = bp;

  while (p != NULL)
  {
    printf("The size of the current block is: %d\n", GET_SIZE(HDRP(p)));
    printf("The current block allocation status is: %d\n", GET_ALLOC(HDRP(p)));
    if (GET_ALLOC(HDRP(p)) != 1)
    {
      printf("The block should be allocated!\n");
      return -1;
    }
    else
    {
      printf("The next block should be free\n");
      // check if the current block is allocated and get its next block
      p = NEXT_BLKP(p);
      printf("The size of the current block is: %d\n", GET_SIZE(HDRP(p)));
      printf("The current block allocation status is: %d\n", GET_ALLOC(HDRP(p)));

      return 1;

      // if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp))) >= new_size)
      // {
      //   set_allocated(bp, new_size);
      //   return bp;
      // }
      // bp = NEXT_BLKP(bp);
    }
  }
  return 1;
}

static void add_page_chunk(void *memory)
{
  // cast memory to a page chunk
  page_chunk *new_page_chunk = (page_chunk *)(memory);

  if (first_page_chunk == NULL)
  {
    new_page_chunk->next = NULL;
    new_page_chunk->prev = NULL;
    first_page_chunk = new_page_chunk;
  }
  else
  {
    // set the first page chunk previous
    first_page_chunk->prev = new_page_chunk;

    // set the new page chunk next
    new_page_chunk->next = first_page_chunk;

    // set the new page chunk previous
    new_page_chunk->prev = NULL;

    // set the first page chunk as the next page chunk
    first_page_chunk = new_page_chunk;
  }
}

static void set_allocated(void *bp, size_t size)
{
  size_t extra_size = GET_SIZE(HDRP(bp)) - size;
  // Check if we can split the page
  if (extra_size > ALIGN(PAGE_OVERHEAD))
  {
    PUT(HDRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
  }
  PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
  PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
}

static void *coalesce(void *bp)
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  // CASE 1: BOTH ALLOCATED
  if (prev_alloc && next_alloc)
  {
    return bp;
  }

  // CASE 2: PREVIOUS ALLOCATED and next not allocated
  else if (prev_alloc && !next_alloc)
  {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  // CASE 3: previous alloacted and next not allocated
  else if (!prev_alloc && next_alloc)
  {
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  // both unallocated
  else
  {
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  return bp;
}
