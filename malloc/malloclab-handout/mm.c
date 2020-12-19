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
/**********************************************************************************/
// VARIABLES AND MACROS FOR SETTING UP BLOCK INFORMATION
typedef size_t block_header;
typedef size_t block_footer;

typedef struct list_node
{
  struct list_node *prev;
  struct list_node *next;
} list_node;

#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))

// Padding(8 bytes) + Prolog Header(8 bytes) + Prolog Footer(8 bytes) + Epilogue(8 bytes) = 32 BYTES
#define PAGE_OVERHEAD 32

// max function from textbook page 857
#define MAX(x, y) ((x) > (y) ? (x) : (y))

//Given a payload pointer, get the header or footer pointer
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

#define DOUBLE_THRESHOLD 0x4000 // 4 PAGES = 16,384 bytes
/*********************************************************************************/
// HELPER FUNCTIONS
/* Set a block to allocated 
 * Update block headers/footers as needed 
 * Update free list if applicable 
 * Split block if applicable 
 */
static void set_allocated(void *b, size_t size);

/* Request more memory by calling mem_map 
 * Initialize the new chunk of memory as applicable 
 * Update free list if applicable 
 */
static void *extend(size_t s);

// /* Coalesce a free block if applicable
//  * Returns pointer to new coalesced block
//  */
static void *coalesce(void *bp);

static void add_to_free_list(void *bp);

static void remove_from_free_list(void *bp);

/*****************************************************************************/

// Global Variables
static list_node *start_free = NULL;
static size_t curr_page_bytes = 0;
static void *pgs = NULL;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  start_free = NULL;
  curr_page_bytes = 4096;
  pgs = NULL;
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  printf("call malloc\n");
  // check if the user requests a size of  0
  if (size == 0)
  {
    return NULL;
  }

  int need_size = MAX(size, sizeof(list_node));
  int new_size = ALIGN(need_size + OVERHEAD);

  printf("aligned size: %d\n", new_size);

  // check if we have an available free block to allocate memory
  if (start_free == NULL)
  {
    // call the extend function to get a new set of pages
    void *new_free = extend(new_size);

    // add the new block to the free list
    add_to_free_list(new_free);
  }

  list_node *curr_free = start_free;

  while (1)
  {
    // // print stuff
    // printf("current_free: %p\n", curr_free);
    // printf("size_avail: %d\n", GET_SIZE(HDRP(curr_free)));

    // if there is available space then allocate the block
    if (GET_SIZE(HDRP(curr_free)) >= new_size)
    {
      set_allocated(curr_free, new_size);
      return (void *)(curr_free);
    }
    // if there is no next free block, break and request more space
    // and add the new node and pointer
    if (curr_free->next == NULL)
    {
      break;
    }
    else
    {
      curr_free = curr_free->next;
    }
  }

  // call the extend function
  void *new_free = extend(new_size);

  // allocate the new block
  set_allocated(new_free, new_size);

  // return pointer to the new memory
  return (void *)(new_free);
}

static void *extend(size_t new_size)
{
  size_t page_bytes = PAGE_ALIGN(new_size);
  printf("page bytes: %d\n", page_bytes);

  void *pgs;

  if (curr_page_bytes <= page_bytes)
  {
    curr_page_bytes = page_bytes;
  }

  // double the pages if we are below the threshold
  if (curr_page_bytes < DOUBLE_THRESHOLD)
  {
    curr_page_bytes *= 2;
  }

  // get the memory associated with the current page bytes
  pgs = mem_map(curr_page_bytes);

  // check to see if mem map returned null
  // since it can return null if it has no space available to satisfy the request
  if (!pgs)
  {
    return NULL;
  }

  void *pg_bytes = pgs;

  // allocate all the information about the blocks
  PUT(pg_bytes, curr_page_bytes);                               // padding block, contains the size of the page
  PUT(pg_bytes + 8, PACK(OVERHEAD, 1));                         // prolog header
  PUT(pg_bytes + 16, PACK(OVERHEAD, 1));                        // prolog footer
  PUT(pg_bytes + 24, PACK(curr_page_bytes - PAGE_OVERHEAD, 0)); // free block header
  // move the pg_bytes pointer by 32 to get to the free block payload pointer
  pg_bytes += 32;
  PUT(FTRP(pg_bytes), PACK(curr_page_bytes - PAGE_OVERHEAD, 0)); // block footer
  PUT(FTRP(pg_bytes) + 8, PACK(0, 1));                           // epilog header

  printf("extend: %p\n", pg_bytes);
  printf("page_size: %d, size_avail: %d\n", GET(pg_bytes - 32), GET_SIZE(HDRP(pg_bytes)));
  printf("epilog: %d, new_free: %d\n", GET_SIZE(FTRP(pg_bytes) + 8), pg_bytes);

  add_to_free_list(pg_bytes); // add the new free block to the linked list

  return pg_bytes;
}

// build a linked list of free blocks
static void add_to_free_list(void *bp)
{
  // cast the void pointer to a list node pointer
  list_node *new_free = (list_node *)(bp);

  printf("add node %p\n", bp);

  if (start_free == NULL)
  {
    new_free->next = NULL;
    new_free->prev = NULL;
    start_free = new_free;
  }
  else
  {
    start_free->prev = new_free;
    new_free->next = start_free;
    new_free->prev = NULL;
    start_free = new_free;
  }
}

static void set_allocated(void *bp, size_t size)
{
  size_t extra_size = GET_SIZE(HDRP(bp)) - size;
  if (extra_size >= PAGE_OVERHEAD)
  {
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    remove_from_free_list(bp);

    // print pointer of allocated block
    printf("alloc: %p\n", bp);

    PUT(HDRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(extra_size, 0));

    // add the new allocated block
    add_to_free_list(bp + size);
  }
  else
  {
    printf("set_alloc_else\n");
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    remove_from_free_list(bp);
  }
}
// remove allocated blocks from free list
static void remove_from_free_list(void *bp)
{
  // cast void pointer to list node pointer and get the next and previous free blocks (if they exist)
  list_node *alloc_block = (list_node *)(bp);
  list_node *next = alloc_block->next;
  list_node *prev = alloc_block->prev;

  // CASE 1: 1 NODE IN THE LINKED LIST
  if (!prev && !next)
  {
    start_free->next = NULL;
    start_free->prev = NULL;
    start_free = NULL;
  }
  // CASE 2: 2 NODE CASE
  else if (!prev && next)
  {
    start_free = start_free->next;
    start_free->prev = NULL;
  }
  // CASE 3: NODE IN THE MIDDLE
  else if (prev && next)
  {
    prev->next = next;
    next->prev = prev;
  }
  // CASE 4: REMOVE THE NODE AT THE END
  else
  {
    prev->next = NULL;
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
  size_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));

  // call the coalesce function
  printf("coalesce\n");
  // returns a pointer to new coalesced block
  void *new_free = coalesce(bp);

  // unmap pages according to flatt video
  void *prev_payload = PREV_BLKP(new_free);
  void *next_payload = NEXT_BLKP(new_free);

  if (GET_SIZE(HDRP(prev_payload)) == OVERHEAD && GET_SIZE(HDRP(next_payload)) == 0) {
    printf("unmap pages\n");
    remove_from_free_list(new_free);
    // get the page size information
    size_t page_size = GET(new_free - 32); // block payload - block head - prolog head - prolog foot - padding
    mem_unmap(new_free - 32, page_size);
  }

  // if (GET_SIZE(HDRP(prev_block)) == OVERHEAD && GET_SIZE(HDRP(next_block)) == 0)
  // {
  //   printf("unmap pages\n");
  //   remove_from_free_list(new_free);
  //   mem_unmap(new_free - PAGE_OVERHEAD, GET_SIZE(HDRP(new_free)) + PAGE_OVERHEAD);
  // }
  printf("get rekt by malloc\n");
}

static void *coalesce(void *bp)
{
  void *prev_payload = PREV_BLKP(bp);
  void *next_payload = NEXT_BLKP(bp);
  size_t prev_alloc = GET_ALLOC(HDRP(prev_payload));
  size_t next_alloc = GET_ALLOC(HDRP(next_payload));
  size_t size = GET_SIZE(HDRP(bp));

  // CASE 1: Next Block and previous block are already allocated
  // take newly allocated free block and add to the free list
  if (prev_alloc && next_alloc)
  {
    printf("enter case 1\n");
    add_to_free_list(bp);
  }
  // CASE 2: Next block is not allocated and previous block is allocated
  else if (prev_alloc && !next_alloc)
  {
    printf("enter case 2\n");
    size += GET_SIZE(HDRP(next_payload));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(next_payload), PACK(size, 0));
    // remove the previous free block from the free list
    remove_from_free_list(next_payload);
    // add the new sized free block to the free list
    add_to_free_list(bp);
  }
  // CASE 3: Next block is allocated and previous block is unallocated
  else if (!prev_alloc && next_alloc)
  {
    printf("enter case 3\n");
    size += GET_SIZE(HDRP(prev_payload));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(prev_payload), PACK(size, 0));
    bp = prev_payload;
  }
  // CASE 4: Next block is not allocated and previous block is not allocated
  else
  {
    printf("enter case 4\n");
    size += (GET_SIZE(HDRP(prev_payload)) + GET_SIZE(HDRP(next_payload)));
    PUT(HDRP(prev_payload), PACK(size, 0));
    PUT(FTRP(next_payload), PACK(size, 0));
    // remove the previous free block from the free list
    remove_from_free_list(next_payload);
    bp = prev_payload;
  }

  return bp;
}