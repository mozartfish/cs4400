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
  free_list = NULL;
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  printf("malloc function\n");

  // check if the user requests a size of  0
  if (size == 0)
  {
    return NULL;
  }

  int need_size = MAX(size, sizeof(list_node));

  printf("need size: %d\n", need_size);

  int new_size = ALIGN(need_size + OVERHEAD);

  printf("aligned size: %d\n", new_size);

  if (free_list == NULL)
  {
    // call th extend function
    extend(52 * new_size);
  }

  list_node *current_free_block = free_list;

  while (1)
  {
    // print stuff
    printf("current_free: %p\n", current_free_block);
    printf("size_avail: %d\n", GET_SIZE(HDRP(current_free_block)));

    if (GET_SIZE(HDRP(current_free_block)) >= new_size)
    {
      set_allocated(current_free_block, new_size);
      return (void *)(current_free_block);
    }
    else if (current_free_block->next == NULL)
    {
      // extend the new size
      extend(52 * new_size);
      current_free_block = free_list;
    }
    else
    {
      current_free_block = current_free_block->next;
    }
  }

  return NULL;
}

static void extend(size_t new_size)
{
  printf("extend function\n");
  printf("new size: %d\n", new_size);

  // get a rounded number of bytes to the nearest page bytes
  int page_bytes = PAGE_ALIGN(new_size);
  printf("page bytes: %d\n", page_bytes);

  // return a pointer to the contiguous block of pages
  void *pgs = mem_map(page_bytes);

  // TEXTBOOK PAGE 858
  PUT(pgs, 0);                                        // Alignment padding 8 Bytes
  PUT(pgs + 8, PACK(OVERHEAD, 1));                    // PROLOG HEADER 8 bytes
  PUT(pgs + 16, PACK(OVERHEAD, 1));                   // PROLOG FOOTER 8 bytes
  PUT(pgs + 24, PACK(page_bytes - PAGE_OVERHEAD, 0)); // BLOCK HEADER 8 bytes
  // move the pages pointer 32 bytes to get past the payload to the footer
  pgs += 32;
  PUT(FTRP(pgs), PACK(page_bytes - PAGE_OVERHEAD, 0)); // BLOCK FOOTER 8 bytes
  PUT(FTRP(pgs) + 8, PACK(0, 1));                      // EPILOG HEADER 8 bytes

  add_to_free_list(pgs);

  printf("extend: %p\n", pgs);
  printf("size: %d, current_avail_size: %d, hdr: %d\n", new_size, page_bytes, GET_SIZE(HDRP(pgs)));
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

static void set_allocated(void *bp, size_t size)
{
  size_t extra_size = GET_SIZE(HDRP(bp)) - size;
  printf("extra size: %d\n", extra_size);

  PUT(HDRP(bp), PACK(size, 1));
  PUT(HDRP(bp), PACK(size, 1));
  remove_from_free_list(bp);

  if (extra_size >= PAGE_OVERHEAD)
  {
    // get the next payload pointer
    void *next_block = NEXT_BLKP(bp);
    PUT(HDRP(next_block), PACK(extra_size, 0));
    PUT(FTRP(next_block), PACK(extra_size, 0));
    // add the new allocated block
    add_to_free_list(bp + size);
  }
  else
  {
    printf("set_allo_else\n");
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    remove_from_free_list(bp);
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

  // unmap map pages according to flatt video
  void *prev_block = PREV_BLKP(new_free);
  void *next_block = NEXT_BLKP(new_free);

  if (GET_SIZE(HDRP(prev_block)) == 16 && (GET_SIZE(HDRP(next_block)) == 0))
  {
    printf("free %p\n", new_free);
    remove_from_free_list(new_free);
    mem_unmap(new_free - PAGE_OVERHEAD, GET_SIZE(HDRP(new_free)) + PAGE_OVERHEAD);
  }
  printf("get rekt by malloc\n");
}

static void *coalesce(void *bp)
{
  void *prev_payload = PREV_BLKP(bp);
  void *next_payload = NEXT_BLKP(bp);
  size_t prev_alloc = GET_ALLOC(HDRP(prev_payload));
  size_t next_alloc = GET_ALLOC(HDRP(next_payload));

  // CASE 1
  if (prev_alloc && next_alloc)
  {
    add_to_free_list(bp);
  }
  // CASE 2
  else if (prev_alloc && !next_alloc)
  {
    size_t size = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(next_payload));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    // remove the previous free block from the free list
    remove_from_free_list(next_payload);
    // add the new sized free block to the free list
    add_to_free_list(bp);
  }
  // CASE 3
  else if (!prev_alloc && next_alloc)
  {
    size_t size = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(prev_payload));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(prev_payload), PACK(size, 0));
    bp = prev_payload;
  }
  // CASE 4
  else
  {
    size_t size = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(prev_payload)) + GET_SIZE(HDRP(next_payload));

    PUT(HDRP(prev_payload), PACK(size, 0));
    PUT(FTRP(prev_payload), PACK(size, 0));
    remove_from_free_list(next_payload);
    add_to_free_list(prev_payload);
    bp = prev_payload;
  }
  return bp;
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
    free_list->next = NULL;
    free_list->prev = NULL;
    free_list = NULL;
  }
  // CASE 2: 2 NODE CASE
  else if (!prev && next)
  {
    free_list = free_list->next;
    free_list->prev = NULL;
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
