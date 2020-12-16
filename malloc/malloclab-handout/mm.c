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

typedef struct list_node {
  struct list_node *prev;
  struct list_node *next;
} list_node;

#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))

// macro for padding as described in the book. Used size_t since that has a size of 8 bytes
#define PADDING (sizeof(size_t))

// Padding + Prolog Header + Prolog Footer + Epilogue
#define PAGE_OVERHEAD (PADDING + sizeof(block_header) + sizeof(block_footer) + sizeof(block_header))

#define WSIZE 8 // size of a word for x86
#define DSIZE 16 // size of a double word for x86

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
static void extend(size_t s);

// /* Coalesce a free block if applicable
//  * Returns pointer to new coalesced block
//  */
static void *coalesce(void *bp);

static void add_to_free_list(void *bp);
static void remove_from_free_list(void *bp);
/*****************************************************************************/


static list_node *free_list = NULL;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  // current_avail = NULL;
  // current_avail_size = 0;

  // // test extend function
  // extend(10);
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  // print the original size requested
  // printf("%zu\n", size);
  // print the size of list node (should be 16 bytes)
  // printf("%zu\n", sizeof(list_node));
  int need_size = MAX(size, sizeof(list_node));



  // print the size we need
  // printf("%d", need_size);
  int new_size = ALIGN(need_size + OVERHEAD);
  // print the aligned new size
  // printf("%d", new_size);

  if (free_list == NULL) {
    // call th extend function
    extend(new_size);
  }

  list_node *current_free_block = free_list;

  while (current_free_block != NULL) {
    if (GET_SIZE(HDRP(current_free_block)) >= new_size) {
      set_allocated(current_free_block, new_size);
      return (void *)(current_free_block);
    }
    if (current_free_block->next == NULL) {
      extend(new_size);
      current_free_block = free_list;
    }
    else {
      current_free_block = current_free_block->next;
    }
  }
  // int newsize = ALIGN(size);
  // void *p;

  // if (current_avail_size < newsize)
  // {
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
void mm_free(void *bp)
{
  size_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));

  // call the coalesce function
  printf("coalesce\n");
  coalesce(bp);
}

static void *coalesce(void *bp) {
  size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  // CASE 1: Next Block and previous block are already allocated
  // take newly allocated free block and add to the free list
  if(prev_alloc && next_alloc) {
    printf("enter case 1\n");
    add_to_free_list(bp);
  }
  // CASE 2: Next block is not allocated and previous block is allocated
  else if (prev_alloc && !next_alloc) {
    printf("enter case 2\n");
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    // remove the previous free block from the free list
    remove_from_free_list(NEXT_BLKP(bp));
    // add the new sized free block to the free list
    add_to_free_list(bp);
  }
  // CASE 3: Next block is allocated and previous block is unallocated
  else if(!prev_alloc && next_alloc) {
    printf("enter case 3\n");
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  // CASE 4: Next block is not allocated and previous block is not allocated
  else {
    printf("enter case 4\n");
    size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    // remove the previous free block from the free list
    remove_from_free_list(NEXT_BLKP(bp));
    bp = PREV_BLKP(bp);
  }

  return bp;
}

static void set_allocated(void *bp, size_t size) {
  size_t csize = GET_SIZE(HDRP(bp));

  // print the size available
  // printf("%zu\n", csize);

  if ((csize - size) >= PAGE_OVERHEAD) {
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    // remove the allocated block
    remove_from_free_list(bp);

    // update the pointer of the next block
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize - size, 0));
    PUT(FTRP(bp), PACK(csize - size, 0));
    add_to_free_list(bp);
  }
  else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}
static void extend(size_t new_size)
{
  // get a rounded number of bytes to the nearest page size
  int page_size_bytes = PAGE_ALIGN(new_size);

  // print the aligned page size
  // printf("%zu\n", page_size_bytes);

  // return a pointer to the contiguous block of pages
  void *pgs = mem_map(page_size_bytes);

  // printf("%zu\n", mem_heapsize() / 4096);

  // TEXTBOOK PAGE 858
  PUT(pgs, 0);                                                      // ALIGNMENT PADDING
  PUT(pgs + (1 * WSIZE), PACK(OVERHEAD, 1));                        // PROLOG HEADER
  PUT(pgs + (2 * WSIZE), PACK(OVERHEAD, 1));                        // PROLOG FOOTER
  PUT(pgs + (3 * WSIZE), PACK(page_size_bytes - PAGE_OVERHEAD, 1)); // NEW FREE BLOCK HEADER
  // increment 32 to get to the free block pointer
  pgs += 32;
  PUT(FTRP(pgs), PACK(page_size_bytes - PAGE_OVERHEAD, 1)); // NEW FREE BLOCK FOOTER
  PUT(FTRP(pgs) + (1 * WSIZE), PACK(0, 1));                 // EPILOG HEADER

  // add node to the explicit free list
  add_to_free_list(pgs);
}

// build a linked list of free blocks
static void add_to_free_list(void *bp)
{
  // cast the void pointer to a list node pointer
  list_node *new_free_block = (list_node *)(bp);
  if (free_list == NULL)
  {
    new_free_block->next = NULL;
    new_free_block->prev = NULL;
    free_list = new_free_block;
  }
  else
  {
    free_list->prev = new_free_block;
    new_free_block->next = free_list;
    new_free_block->prev = NULL;
    free_list = new_free_block;
  }
}

// remove allocated blocks from free list
static void remove_from_free_list(void *bp)
{
  // cast void pointer to list node pointer and get the next and previous free blocks (if they exist)
  list_node *alloc_block = (list_node *)(bp);
  list_node *alloc_block_next = alloc_block->next;
  list_node *alloc_block_prev = alloc_block->prev;

  // CASE 1: ALLOCATED BLOCK IS THE FREE BLOCK
  // ACCORDING TO LIFO POLICY IN TEXTBOOK
  if (alloc_block == free_list)
  {
    // CASE 1: FIRST FREE BLOCK HAS ZERO CHILDREN
    if (alloc_block_next == NULL && alloc_block_prev == NULL)
    {
      alloc_block->next = NULL;
      alloc_block->prev = NULL;
      alloc_block = NULL;
    }
    // CASE 2: FIRST FREE BLOCK HAS NEXT
    else if (alloc_block_next != NULL && alloc_block_prev == NULL)
    {
      free_list = alloc_block_next;
      free_list->prev = NULL;
    }
  }
  // CASE 2: ALLOC BLOCK IS IN THE MIDDLE OF THE LINKED LIST
  else if (alloc_block_next != NULL && alloc_block_prev != NULL)
  {
    // set the previous pointers
    alloc_block_prev->next = alloc_block_next;

    // set the next pointers
    alloc_block_next->prev = alloc_block_prev;
  }
  // CASE 3: ALLOC BLOCK IS AT THE END OF THE LINKED LIST
  else if (alloc_block_next == NULL && alloc_block_prev != NULL)
  {
    alloc_block_prev->next = NULL;
  }
}
