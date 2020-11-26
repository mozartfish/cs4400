/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

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
static void *coalesce(void *bp);
static void add_node(void *pgs);
/****************************************************************************************************/
/* GLOBAL VARIABLES AND DATA STRUCTURES */
typedef size_t block_header;
typedef size_t block_footer;

typedef struct list_node
{
  struct list_node *next;
  struct list_node *prev;
} list_node;

// pointer to keep track of the first block payload
static void *first_bp;

// pointer to keep track of the subsequent block payloads
static void *bp;

// global pointer to the first page chunk
static list_node *first_page_chunk = NULL;
/***************************************************************************************************/
void *current_avail = NULL;
int current_avail_size = 0;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  first_bp = NULL;
  bp = NULL;
  current_avail = NULL;
  current_avail_size = 0;

  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  // printf("The original number of bytes requested by the user: %d\n", size);
  // variable to store the new size which is aligned to take care of the overhead
  int new_size = ALIGN(size + OVERHEAD);

  // printf("The new aligned size requested by the user: %d\n", new_size);

  // check if there is a page chunk available
  if (first_page_chunk == NULL)
  {
    extend(new_size);
  }

  while (1) {
    // set the bp pointer to the first bp
    bp = first_bp;
    while (GET_SIZE(HDRP(bp)) != 0)
    {
      printf("The size available is : %d", GET_SIZE(HDRP(bp)));
      printf("The new size: %d", new_size);
      if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp))) >= new_size)
      {
        set_allocated(bp, new_size);
        return bp;
      }
      else
      {
        bp = NEXT_BLKP(bp);
      }
    }
    exit(0);
  }
  return NULL;
  // int newsize = ALIGN(size);
  // void *p;

  // if (current_avail_size < newsize) {
  //   current_avail_size = PAGE_ALIGN(newsize);
  //   current_avail = mem_map(current_avail_size);
  //   if (current_avail == NULL)
  //     return NULL;
  // }

  // p = current_avail;
  // current_avail += newsize;
  // current_avail_size -= newsize;

  // return p;
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
  current_avail_size = PAGE_ALIGN(new_size);

  // get a number p bytes that are equivalent to page_chunk_size
  current_avail = mem_map(current_avail_size);

  // add a page chunk to the linked list
  add_node(current_avail);

  current_avail += sizeof(list_node);                                   // move the p pointer past the first 16 bytes since that's assigned for the pages
  PUT(current_avail, 0);                                                // padding of 8 bytes
  PUT(current_avail + 8, PACK(OVERHEAD, 1));                            // PROLOGUE Header;
  PUT(current_avail + 16, PACK(OVERHEAD, 1));                           // PROLOGUE FOOTER;
  PUT(current_avail + 24, PACK(current_avail_size - PAGE_OVERHEAD, 0)); // Payload Header
  first_bp = current_avail + 32;                                        // Payload memory
  PUT(FTRP(first_bp), PACK(current_avail_size - PAGE_OVERHEAD, 0));     // Payload Footer
  PUT(FTRP(first_bp) + 8, PACK(0, 1));                                  // EPILOGUE Header
}
static void add_node(void *pgs)
{
  // cast memory to a page chunk
  list_node *new_page_chunk = (list_node *)(pgs);

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
static void set_allocated(void *bp, size_t asize)
{
  size_t csize = GET_SIZE(HDRP(bp));
  if (csize - asize >= PAGE_OVERHEAD)
  {
    put(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize - asize, 0));
    printf("next size %d\n", GET_SIZE(HDRP(bp)));
    printf("next alloc %d\n", GET_ALLOC(HDRP(bp)));
    PUT(FTRP(bp), PACK(csize - asize, 0));
  }
  else
  {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}
