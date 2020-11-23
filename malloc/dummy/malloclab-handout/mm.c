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

// 24 bytes
typedef struct footer_block {
  size_t block_size; // variable for indicating the size
  size_t allocated; // variable for indicating whether the block has been allocated
  struct footer_block *next; // pointer to the next footer block 
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
void *current_avail = NULL; // global variable for keeping of the memory returned by mem_map
int current_avail_size = 0; // the size of the available pages
/**********************************************************************************************/
/* HELPER FUNCTIONS */
static void extend(size_t s);
static void add_pages(void *page_amt);
/*********************************************************************************************/
/*MACROS FOR DEFINING CONSTANTS */
#define OVERHEAD (sizeof(page_node) + sizeof(header_block) + sizeof(footer_block) + sizeof(header_block) + sizeof(footer_block) + sizeof(header_block))
/********************************************************************************************/

/* 
 * mm_init - initialize the malloc package.
 */

mm_init(void)
{
  // initialize structs
  heap = NULL;
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

/* Request more memory by calling mem_map
*  Initialize the new chunk of memory as applicable
*  Update free list if applicable
*/
static void extend(size_t s) {
  // first align the pages
  current_avail_size = PAGE_ALIGN(s);
  current_avail = mem_map(current_avail_size); // currently a void * pointer

  // update the page linked list
  add_pages(current_avail);

  // cast current available to a char*
  char *page_struct = (char *)current_avail;

  // cast 24 bytes onwards as a block header for prologue header
  header_block *prologue_header = (header_block *)(page_struct + 24);
  prologue_header->block_size = 48;
  prologue_header->allocated = 1;
  prologue_header->next = NULL;

  // cast the next 24 bytes onwards as a block footer for prologue footer
  footer_block *prologue_footer = (footer_block *)(page_struct + 48);
  prologue_footer->block_size = 48;
  prologue_footer->allocated = 1;
  prologue_footer->next = NULL;

  // cast the next 24 bytes onward as a block header for the payload header
  header_block *payload_header = (header_block *)(page_struct + 72);
  payload_header->block_size = current_avail_size - OVERHEAD;
  payload_header->allocated = 0;
  payload_header->next = NULL;


// cast the second to last 24 bytes as a block footer for the payload footer
  footer_block *payload_footer = (footer_block *)(page_struct - 48);
  payload_footer->block_size = current_avail_size - OVERHEAD;
  payload_footer->allocated = 0;
  payload_footer->next = NULL;
  
  // cast the last 24 bytes as header for the epilogue
  header_block *epilogue = (header_block *)(page_struct - 24);
  epilogue->block_size = 0;
  epilogue->allocated = 1;
  epilogue->next = NULL;

  // // update the current available size
  // current_avail_size -= sizeof(header_block);

  // 24 bytes are reserved for linked list information
  // 24 bytes reserved for the prologue bytes reserved for the prologue header
  // 24 bytes reserved for the prologue footer
  // 24 bytes reserved for the epilogue header
}

static void add_pages(void *page_amt) {
  // convert the page amount to a node
  page_node *new_page_chunk = (page_node *)page_amt;

  // conver the page amount to char
  char *new_page_chunk_byte = (char *)page_amt;

  // CASE 1: HEAP IS NULL
  if (heap == NULL) {
    heap = new_page_chunk;
    heap->next = NULL;
    heap->prev = NULL;
    heap->bytes_avail = new_page_chunk_byte - OVERHEAD;
  }
  else {
    new_page_chunk->next = heap;
    new_page_chunk->prev = NULL;
    new_page_chunk->bytes_avail = new_page_chunk_byte - OVERHEAD;
    heap = new_page_chunk;
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}
