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
typedef struct page_node
{
  struct page_node *next;
  struct page_node *prev;
} page_node;

#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))
#define PADDING (sizeof(size_t))                                                                                         // 8 bytes for padding
#define PAGE_OVERHEAD (sizeof(page_node) + PADDING + sizeof(block_header) + sizeof(block_footer) + sizeof(block_header)) // 48 bytes
#define WSIZE 8                                                                                                          // word
#define DSIZE 8                                                                                                          // double word

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
// static void *coalesce(void *bp);
/*****************************************************************************/

void *current_avail = NULL;
int current_avail_size = 0;
static page_node *first_pg_chunk = NULL;

static void extend(size_t new_size)
{
  // get a group of contiguous pages from mem_map
  current_avail_size = PAGE_ALIGN(new_size);

  // print the aligned page size
  printf("%zu\n", current_avail_size);

  // mem map returns a pointer so printing the size will return 8
  current_avail = mem_map(current_avail_size);

  // if mem map returns null then return null
  if (current_avail == NULL)
  {
    return;
  }

  void *contig_pgs = current_avail;

  printf("%zu\n", mem_heapsize() % 4096);

  // add information for the bytes
  // move past the first 16 bytes allocated for the page pointers
  contig_pgs = contig_pgs + 16;

  PUT(contig_pgs, 0);                                                         // add padding of 8 bytes
  PUT(contig_pgs + (1 * WSIZE), PACK(DSIZE, 1));                              // Prologue Header
  PUT(contig_pgs + (2 * WSIZE), PACK(DSIZE, 1));                              // Prologue Footer
  PUT(contig_pgs + (3 * WSIZE), PACK(current_avail_size - PAGE_OVERHEAD, 0)); // Header
  contig_pgs = contig_pgs + 32;
  PUT(FTRP(contig_pgs), PACK(current_avail_size - PAGE_OVERHEAD, 0)); // Footer
  PUT(FTRP(contig_pgs) + WSIZE, PACK(0, 1));                          // Epilogue Header
  add_pages(contig_pgs);                                              // add the page node to the linked list
}

// build page linked list
static void add_pages(void *pg)
{
  // cast pg to page node
  page_node *new_pg_chunk = (page_node *)(pg);

  if (first_pg_chunk == NULL)
  {
    new_pg_chunk->next = NULL;
    new_pg_chunk->prev = NULL;
    first_pg_chunk = new_pg_chunk;
  }
  else
  {
    // set the first page next
    first_pg_chunk->next = new_pg_chunk;

    // set the new page chunk previous to first page chunk
    new_pg_chunk->prev = first_pg_chunk;

    // set the new page chunk next to null
    new_pg_chunk->next = NULL;

    /* This code is for the explicit linked list version */
    // // set the first page chunk previous
    // first_pg_chunk->prev = new_pg_chunk;

    // // set the new page chunk to previous
    // new_pg_chunk->next = first_pg_chunk;

    // // set the new page chunkj previous
    // new_pg_chunk->prev = NULL;

    // // set the first page chunk as the new page chunk
    // first_pg_chunk = new_pg_chunk;
  }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  current_avail = NULL;
  current_avail_size = 0;
  first_pg_chunk = NULL;
  // test extend function with malloc
  malloc(10);
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{

  // check if the user requests 0
  if (size == 0)
  {
    return NULL;
  }

  int newsize = ALIGN(size + PAGE_OVERHEAD);

  // check if there is a page available
  if (first_pg_chunk == NULL)
  {
    extend(newsize);
  }

  // cast the page node pointer to void to get to the first block header
  page_node *current_pg = first_pg_chunk;
  while (current_pg != NULL)
  {
    void *block_start = (void *)(current_pg);
    block_start = block_start + sizeof(page_node) + OVERHEAD + sizeof(block_header); // page_node pointers + prolog overhead + block header to payload
    printf("%zu\n", GET_SIZE(HDRP(block_start)));
    exit(1);
  }

  // if (current_avail_size < newsize)-[]
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
void mm_free(void *ptr)
{
}
