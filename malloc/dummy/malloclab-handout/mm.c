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
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

/***************************************************************************************/
/*DEFINE DATA STRUCTURES FOR MALLOC */
// 24 bytes 
typedef struct header_block {
  size_t block_size; // variable for indicating the size
  size_t allocated; // variable for indicating whether the block has been set
  struct header_block *next; // pointer to the next header block
} header_block;

// 16 bytes
typedef struct footer_block {
  size_t block_size; // variable for indicating the size
  size_t allocated; // variable for indicating whether the block has been allocated
} footer_block;

// 24 bytes
typedef struct page_node {
  struct page_node *next; // pointer to the next set of pages in the memory list
  struct page_node *prev; // pointer to the previous set of pages in the memory list
  size_t bytes_avail; // variable for keeping track of the number of bytes used in a chunk of pages
} page_node;
/**********************************************************************************************/
/* GLOBAL VARIABLES */
static page_node *heap = NULL; // global variable for keeping track of the heap
void *mem_pages = NULL; // global variable for keeping track of the pages in use 

void *current_avail = NULL;
int current_avail_size = 0;
/**********************************************************************************************/
/* HELPER FUNCTIONS */
static void extend(size_t s);
static void add_pages(void *page_amt);
/*********************************************************************************************/

/* 
 * mm_init - initialize the malloc package.
 */

mm_init(void)
{
  // initialize structs
  heap = NULL;
  mem_pages = NULL;
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
  int newsize = ALIGN(size);
  void *p;
  
  if (current_avail_size < newsize) {
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
