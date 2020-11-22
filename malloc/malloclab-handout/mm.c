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
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

/* represent the header and the footer of a block */
typedef size_t block_header; 
typedef size_t block_footer;

/*Compute the overhead of the block header and footer */
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
/* HELPER FUNCTIONS */

/* Request more memory by calling mem_map
*  Initialize the new chunk of memory as applicable
*  Update free list if applicable
*/
static void extend(size_t s);

/* Function that adds nodes to the doubly linked list
* that is keeping track of memory 
*/
static void add_page_chunk(void *page_chunk);

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
/****************************************************************************************************/

/* GLOBAL VARIABLES AND DATA STRUCTURES */

// pointer to the memory that is currently available in a page chunk
void *current_avail = NULL;

// the size of the of the memory that is currently available in a page chunk
int current_avail_size = 0;

/* linked list structure for keeping track of the memory for use with memmap*/
typedef struct mem_node {
  struct mem_node *dummy; // pointer for aligning on 16 byte boundary
  struct mem_node *next;
  struct mem_node *prev;
} mem_node;

/* global variable for keeping track of start of the heap */
static mem_node *heap = NULL;


/***************************************************************************************************/

/* 
 * mm_init - initialize the malloc package.
 */
int
mm_init(void)
{
  current_avail = NULL;
  current_avail_size = 0;
  heap = NULL;
  // start_chunk = NULL;
  // end_chunk = NULL;

  // int new_size = ALIGN(10);
  // current_avail_size = PAGE_ALIGN(new_size);
  // extend(current_avail_size);

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
    // CASE 1: HEAP DOES NOT EXIST
    // allocate some memory for the heap for initialization
    // and then perform the malloc
    if (heap == NULL) {
      extend(new_size);
    }
    // update the current available size by align by aligning
    current_avail_size = PAGE_ALIGN(new_size);
    // extend(current_avail_size);
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
  // const int byte_threshold = 1 << 8; // 2^8 = 256 bytes
  int new_avail_size = 0;
  void *new_avail = NULL;

  // if (s > byte_threshold)
  // {
  //   new_avail_size = PAGE_ALIGN(s * 4);
  //   new_avail = mem_map(new_avail_size);

  //   // update the page linked list
  //   add_page_chunk(new_avail);

    new_avail_size = PAGE_ALIGN(s);
    new_avail = mem_map(new_avail_size);

    // update the page linked list
    add_page_chunk(new_avail);

    // cast page to char* to update the prologue,epilogue, header and footer pages
    char *page_info = (char *)new_avail;

    // define some constants for alignment
    const int page_pro_header = page_info + 24;
    const int page_pro_footer = page_info + 32;
    const int payload_data = page_info + 48;
    const int new_payload_size = new_avail_size - 40;
    const int page_epi_header = new_avail_size - 8;
    const int new_payload_footer = page_epi_header - 8;

    // PROLOGUE HEADER
    PUT(page_pro_header, PACK(OVERHEAD, 1));

    // PROLOGUE FOOTER
    PUT(page_pro_footer, PACK(OVERHEAD, 1));

    // PAYLOAD HEADER
    PUT(HDRP(payload_data), PACK(new_payload_size, 0));

    // EPILOGUE
    PUT(page_epi_header, PACK(0, 1));

    // PAYLOAD FOOTER
    PUT()


    // Request slightly more than what the user requested to we make fewer calls to
    // mmap similar to how array lists avoid copying elements over in java
    //   current_avail_size = PAGE_ALIGN(s * 15);
    //   current_avail = mem_map(current_avail_size);

    //   // add the new chunk to the memory linked list
    //   mem_chunk_node *new_chunk = (mem_chunk_node *)current_avail;
    //   add_page_chunk(new_chunk);

    //   // add prolog header, prolog footer, payload and epilog header to memory++
    //   char *g = (char *)current_avail;

    //   // PROLOG HEADER HEADER
    //   // OVERHEAD is 16 bytes
    //   PUT(g, PACK(OVERHEAD, 1));

    //   // PROLOG FOOTER
    //   // overhead is 16 bytes
    //   PUT(g + 8, PACK(OVERHEAD, 1));

    //   // PAYLOAD HEADER
    //   PUT(g + 16, PACK(current_avail_size - 32, 0));

    //  // PAYLOAD FOOTER
    //   PUT(g + current_avail_size - 16, PACK(current_avail_size - 32, 0));

    //   // EPILOG HEADER
    //   PUT(g + current_avail_size - 8, PACK(0, 1));
}

static void add_page_chunk(void *page_chunk)
{
  mem_node *new_chunk = (mem_node *)page_chunk;

  // CASE 1: HEAP IS NULL
  if (heap == NULL) {
    heap = new_chunk;
    heap->next = NULL;
    heap->prev = NULL;
    heap->dummy = NULL;
  }

  else {
    // CASE 2: HEAP IS NOT NULL

    // add chunk node to the front of the linked list
    // set the next chunk next to the current heap head
    new_chunk->next = heap;

    // set the previous of the new chunk to null
    new_chunk->prev = NULL;

    // update the dummy of the new_chunk
    new_chunk->dummy = NULL;

    // set the previous of heap to the new chunk
    heap->prev = new_chunk;

    // set the head of the heap to the new chunk
    heap = new_chunk;
  }

  // mem_chunk_node *new_chunk = (mem_chunk_node *)page_chunk;

  // // CASE 1: START CHUNK IS NULL 
  // if (start_chunk == NULL)
  // {
  //   start_chunk = new_chunk;
  //   end_chunk = new_chunk;
  //   start_chunk->next = NULL;
  //   start_chunk->prev = NULL;

  //   // SET THE END CHUNK TO NULL
  //   end_chunk->next = NULL;
  //   end_chunk->prev = NULL;
  // }
  // else
  // {
  //   // update the last node
  //   new_chunk->prev = end_chunk;
  //   end_chunk->next = new_chunk;
  //   end_chunk = new_chunk;
  //   end_chunk->next = NULL;
  // }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

