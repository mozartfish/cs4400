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
 * according to Luis Ceze's Lectures 1-4 https://www.youtube.com/playlist?list=PL0oekSefhQVJdk0hSRu6sZ2teWM740NtL
 *  and in class:
 * 1) Explicit free list
 * 2) coalesce free blocks
 * 3) unmapping unused pages
 * 4) doubling chunk size 
 * 5) LIFO / Addressed Ordered By Policy
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
typedef struct memory_list{
  struct memory_list* next; 
  struct memory_list* prev;
}memory_list;

/* HELPER FUNCTIONS */

/* Request more memory by calling mem_map
*  Initialize the new chunk of memory as applicable
*  Update free list if applicable
*/
static void extend(size_t s);

/* Function that adds nodes to the doubly linked list
* that is keeping track of memory 
*/
static void mem_next(void *page_chunk);

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
/*************************************************************************************/

/* Request more memory by calling mem_map
*  Initialize the new chunk of memory as applicable
*  Update free list if applicable
*/
static void extend(size_t size) {
  current_avail_size = PAGE_ALIGN(size);
  current_avail = mem_map(current_avail_size);

  
}

static void mem_next(void *page_chunk) {
  memory_list * new_chunk = (memory_list *)page_chunk;

  // check if the start is null 
  if (mem_start == NULL) {
    mem_start = new_chunk;
    mem_start->next = NULL;
    mem_start->prev = NULL;
    mem_end->next = NULL;
    mem_end->prev = mem_start;
  }
  else {
    // update the pointers of the new
    new_chunk->next = NULL;
    new_chunk->prev = mem_end->prev;
    mem_end = new_chunk;
  }
}

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

// pointer to the memory that is currently available
void *current_avail = NULL;

// the size of the of the memory that is currently available
int current_avail_size = 0;

// global pointers for keeping track of the start and end of the list
static memory_list *mem_start = NULL;
static memory_list *mem_end = NULL;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  current_avail = NULL;
  mem_start = NULL;
  mem_end = NULL;
  current_avail_size = 0;
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  // check if the size given is 0 
  if (size == 0) {
    return NULL;
  }

  int new_size = ALIGN(size);

  // print the new size
  printf("The new size: %d\n", new_size);
  /*
   * THE ORIGINAL ORIGINAL CODE GIVEN
  */
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

