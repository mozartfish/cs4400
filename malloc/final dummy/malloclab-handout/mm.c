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
/*************************************************************************************************/
// Helper Macros
// This assumes you have a struct or typedef called "block_header" and "block_footer"
#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))

// Given a payload pointer, get the header or footer pointer
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - OVERHEAD)

// Given a payload pointer, get the next or previous payload pointer
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-OVERHEAD))

// Given a pointer to a header, get or set its value
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

// Combine a size and alloc bit
#define PACK(size, alloc) ((size) | (alloc))

// Given a header pointer, get the alloc or size
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_SIZE(p) (GET(p) & ~0xF)

// overall overhead for a page (implicit list implementation)
#define PAGE_OVERHEAD 48 // prolog header + prolog footer + epilog header + padding + chunk pointers
/***********************************************************************************/
/* HELPER FUNCTIONS */
static void extend(size_t new_size);
// static void set_allocated(void *bp, size_t size);
// static int heap_checker(void *bp);
// static void *coalesce(void *bp);
static void add_page_node(void *pg);
/****************************************************************************************************/
// GLOBAL VARIABLES
typedef size_t block_header;
typedef size_t block_footer;
typedef struct page_node
{
  struct page_node *next;
  struct page_node *prev;
} page_node;

static void *first_bp = NULL;
static void *bp = NULL;
static page_node *first_page_chunk = NULL;
void *current_avail = NULL;
int current_avail_size = 0;

/****************************************************************************************************/

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  current_avail = NULL;
  current_avail_size = 0;
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
  // printf("original size: %d\n", size);
  int newsize = ALIGN(size + OVERHEAD);
  printf("aligned size: %d\n", newsize);

  extend(newsize);

  void *p;

  if (current_avail_size < newsize)
  {
    current_avail_size = PAGE_ALIGN(newsize);
    current_avail = mem_map(current_avail_size);
    if (current_avail == NULL)
      return NULL;
  }

  p = current_avail;
  current_avail += newsize;
  current_avail_size -= newsize;

  return p;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

static void extend(size_t new_size)
{
  // get a chunk of pages that satisfies the new requested size
  size_t current_size = PAGE_ALIGN(new_size);
  printf("page align : %d\n", current_size);
}

static void add_page_node(void *pg)
{
  // cast memory to a page chunk
  page_node *new_page_chunk = (page_node *)(pg);

  if (new_page_chunk == NULL)
  {
    new_page_chunk->next = NULL;
    new_page_chunk->prev = NULL;
    new_page_chunk = new_page_chunk;
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
