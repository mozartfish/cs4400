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
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

/************************************************************************************/

/* create a typedef called block_header and block_footer according to assignment hints */
typedef size_t block_header; 
typedef size_t block_footer;

#define OVERHEAD (sizeof(block_header)+sizeof(block_footer))

/* Given a payload pointer, get the header or footer pointer */
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-OVERHEAD)

/* Given a payload pointer, get the next or previous payload pointer */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-OVERHEAD))
/*************************************************************************************/

/* Given a pointer to a header, get or set its value */
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

/* Combine a size and alloc bit */
#define PACK(size, alloc) ((size) | (alloc))

/* Given a header pointer, get the alloc or size */
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_SIZE(p) (GET(p) & ~0xF)

/**************************************************************************************/

/* define a doubly linked list for keeping track of the available heap */
typedef struct mem_chunk_node{
  struct mem_chunk_node* next; 
  struct mem_chunk_node* prev;
}mem_chunk_node;

/* HELPER FUNCTIONS */

/* Request more memory by calling mem_map
*  Initialize the new chunk of memory as applicable
*  Update free list if applicable
*/
static void extend(size_t s);

/* Function that adds nodes to the doubly linked list
* that is keeping track of memory 
*/
static void add_chunk(void *page_chunk);

/* Set a block to allocated
*  Update block headers/footers as needed
*  Update free list if applicable
*  Split block if applicable
*/
// static void set_allocated(void *b, size_t size);

/* Coalesce a free block if applicable
*  Returns pointer to new coalesced block
*/
// static void *coalesce(void *bp);

// GLOBAL VARIABLES

// pointer to the memory that is currently available in a page chunk
void *current_avail = NULL;

// the size of the of the memory that is currently available in a page chunk
int current_avail_size = 0;

// global pointer that represents the start of the free list
// represents the start of the doubly linked list for the page stuff
static mem_chunk_node *start_chunk = NULL;
static mem_chunk_node *end_chunk = NULL;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  current_avail = NULL;
  current_avail_size = 0;
  start_chunk = NULL;
  end_chunk = NULL;
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{

  /** THE ORIGINAL STARTER CODE GIVEN THAT WORKS */
  // variable that stores the aligned size of the amt of bytes requested by the user
  int new_size = ALIGN(size);

  // pointer that will be returned to the user that points to a contiguous chunk of memory that fits the size requested by the user
  void *p;

  // if there is not enough space available to satisfy the user request then request for memory by calling mem_map (or extend)
  if (current_avail_size < new_size)
  {
    // update the current available size by align by aligning
    //  
    current_avail_size = PAGE_ALIGN(new_size); 
    current_avail = mem_map(current_avail_size);
    if (current_avail == NULL)
      return NULL;
  }

  p = current_avail;
  current_avail += new_size;
  current_avail_size -= new_size;

  return p;
}

static void extend(size_t s) {
  // Request slightly more than what the user requested to we make fewer calls to
  // mmap similar to how array lists avoid copying elements over in java
  current_avail_size = PAGE_ALIGN(s * 15);
  current_avail = mem_map(current_avail_size);

  // add the new chunk to the memory linked list
  mem_chunk_node *new_chunk = (mem_chunk_node *)current_avail;
  add_chunk(new_chunk);

  // add prolog header, prolog footer, payload and epilog header to memory++
  char *g = (char *)current_avail;

  // PROLOG HEADER HEADER
  // OVERHEAD is 16 bytes
  PUT(g, PACK(OVERHEAD, 1));

  // PROLOG FOOTER
  // overhead is 16 bytes
  PUT(g + 8, PACK(OVERHEAD, 1));

  // PAYLOAD HEADER
  PUT(g + 16, PACK(current_avail_size - 32, 0));

 // PAYLOAD FOOTER
  PUT(g + current_avail_size - 16, PACK(current_avail_size - 32, 0));

  // EPILOG HEADER
  PUT(g + current_avail_size - 8, PACK(0, 1));
}

static void add_chunk(void *page_chunk)
{
  mem_chunk_node *new_chunk = (mem_chunk_node *)page_chunk;

  // CASE 1: START CHUNK IS NULL 
  if (start_chunk == NULL)
  {
    start_chunk = new_chunk;
    end_chunk = new_chunk;
    start_chunk->next = NULL;
    start_chunk->prev = NULL;

    // SET THE END CHUNK TO NULL
    end_chunk->next = NULL;
    end_chunk->prev = NULL;
  }
  else
  {
    // update the last node
    new_chunk->prev = end_chunk;
    end_chunk->next = new_chunk;
    end_chunk = new_chunk;
    end_chunk->next = NULL;
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

