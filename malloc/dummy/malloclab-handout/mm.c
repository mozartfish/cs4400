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
/**********************************************************************************************/
/* HELPER FUNCTIONS */
static void extend(size_t s);
static void add_to_free_list(void *bp);
static void remove_from_free_list(void *bp);
static void set_allocated(void *bp, size_t size);
/*********************************************************************************************/
/*MACRO CONSTANTS*/
// This assumes you have a struct or typedef called "block_header" and "block_footer"
#define OVERHEAD (sizeof(block_header)+sizeof(block_footer))

// Given a payload pointer, get the header or footer pointer
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-OVERHEAD)

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

/********************************************************************************************/
/*DEFINE DATA STRUCTURES FOR MALLOC */
typedef size_t block_header;
typedef size_t block_footer;

// 16 bytes
// typedef struct header_block
// {
//   size_t block_size; // variable for indicating the size
//   size_t allocated;  // variable for indicating whether the block has been set
// } block_header;

// // 16 bytes
// typedef struct footer_block
// {
//   size_t block_size; // variable for indicating the size
//   size_t allocated;  // variable for indicating whether the block has been allocated
// } block_footer;

/*Doubly Linked List Data Structure for keeping track of the free blocks */
typedef struct list_node
{
  struct list_node *prev;
  struct list_node *next;
} list_node;
/*********************************************************************************************/
/* GLOBAL VARIABLES */
static list_node *free_list = NULL; // global variable for keeping track of the free blocks
void *current_avail = NULL;         // global variable for keeping of the memory returned by mem_map
int current_avail_size = 0;         // the size of the available pages
/**********************************************************************************************/
/* 
 * mm_init - initialize the malloc package.
 */

int mm_init(void)
{
  // initialize structs
  free_list = NULL;
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

  // check if the free list is empty
  if (free_list== NULL) {
    extend(newsize);
  }

  // traverse the free list looking for space
  // create a new list node variable that starts at the free list
  // since the free list gets updated
  list_node *start = free_list;

  // look for space continously
  while (1) {
    // get the header for the current free block in the list
    char *header = HDRP(start);

    // get the size in the header
    size_t available_size = GET_SIZE(header);

    if (available_size >= size) {
      // allocate the space
      p = (void *)(start);
      return p;
    }

    // if the current free block does not have enough space for the request check the next free block
    if (free_list->next != NULL) {
      start = free_list->next;
    }

    // if the next free block is null request for more space
    if (free_list->next == NULL) {
      extend(newsize);
      // allocate space in the new head of the free list
      start = free_list;
    }
  }

  // page_chunk *current_page_chunk = heap;

  // header_block *start = (header_block *) (current_page_chunk + sizeof(page_chunk) + sizeof(header_block) + sizeof(footer_block)); // 24 bytes * 3 = 72 -> start at byte 72
  // header_block *end = (header_block *) (current_page_chunk + current_avail_size - sizeof(header_block));

  // while (start < end) {
  //   // if the size of the header is satisfies the request
  //   if (GET_SIZE(start) >= newsize) {
  //     // check if the block is allocated
  //     if (GET_ALLOC(start) == 1) {
  //       start
  //     }
  //   }
  // }
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

/* Request more memory by calling mem_map
*  Initialize the new chunk of memory as applicable
*  Update free list if applicable
*/
static void extend(size_t s) {
  // align the number of bytes for the pages requested
  current_avail_size = PAGE_ALIGN(s);

  // request some memory
  current_avail = mem_map(current_avail_size); // currently a void * pointer

  // cast current avail to char* to pack the information for the 
  // prologue header, prologue footer, payload header, payload footer, epilogue header
  char *total_bytes = (char *)(current_avail);

  // Constants for the different parts of the page list chunk
  char *prologue_header = total_bytes + 8;
  char *prologue_footer = total_bytes + 16;
  char *payload_header = total_bytes + 24;
  char *payload_pointer = total_bytes + 32;
  char *payload_footer = FTRP(payload_pointer);
  char *epilogue_header = payload_footer + 8;

  // PROLOGUE is a 32-byte allocated block consisting of header and footer
  PUT(prologue_header, PACK(16, 1));
  PUT(prologue_footer, PACK(16, 1));
  PUT(payload_header, PACK(current_avail_size - 32, 0));
  PUT(payload_footer, PACK(current_avail_size - 32, 0));
  PUT(epilogue_header, PACK(0, 1));

  // add the node to the free block linked list
  add_to_free_list(payload_pointer);

  //   // typecast current available to set the number of bytes from page_align size
  //   page_chunk *page_struct = (page_chunk *)(current_avail);
  //   page_struct->bytes_avail = current_avail_size;

  //   // update the page linked list
  //   add_pages(current_avail);

  //   // cast current available to a char*
  //   // char *page_struct = (char *)current_avail;

  //   // cast 24 bytes onwards as a block header for prologue header
  //   header_block *prologue_header = (header_block *)(page_struct + sizeof(page_chunk));
  //   prologue_header->block_size = 48;
  //   prologue_header->allocated = 1;
  //   prologue_header->next = NULL;

  //   // cast the next 24 bytes onwards as a block footer for prologue footer
  //   footer_block *prologue_footer = (footer_block *)(page_struct + sizeof(page_chunk) + sizeof(header_block));
  //   prologue_footer->block_size = 48;
  //   prologue_footer->allocated = 1;
  //   prologue_footer->next = NULL;

  //   // cast the next 24 bytes onward as a block header for the payload header
  //   header_block *payload_header = (header_block *)(page_struct + sizeof(page_chunk) + sizeof(header_block) + sizeof(footer_block));
  //   payload_header->block_size = current_avail_size - OVERHEAD;
  //   payload_header->allocated = 0;
  //   payload_header->next = NULL;

  // // cast the second to last 24 bytes as a block footer for the payload footer
  //   footer_block *payload_footer = (footer_block *)(current_avail_size - sizeof(header_block) - sizeof(footer_block));
  //   payload_footer->block_size = current_avail_size - OVERHEAD;
  //   payload_footer->allocated = 0;
  //   payload_footer->next = NULL;

  //   // cast the last 24 bytes as header for the epilogue
  //   header_block *epilogue = (header_block *)(current_avail_size - sizeof(header_block));
  //   epilogue->block_size = 0;
  //   epilogue->allocated = 1;
  //   epilogue->next = NULL;

  // // update the current available size
  // current_avail_size -= sizeof(header_block);

  // 24 bytes are reserved for linked list information
  // 24 bytes reserved for the prologue bytes reserved for the prologue header
  // 24 bytes reserved for the prologue footer
  // 24 bytes reserved for the epilogue header
}

/*
* Function that takes a block
* and adds it to the free list
*/
static void add_to_free_list(void *bp) {
  // cast the block pointer to a node pointer
  list_node *new_block = (list_node *)(bp);

  // CASE 1: THE FREE LIST IS EMPTY
  if (free_list == NULL) {
    free_list = new_block;
    free_list->next = NULL;
    free_list->prev = NULL;
  }
  // CASE 2: FREE BLOCK LIST IS NOT EMPTY
  else {
    free_list->prev = new_block;
    new_block->next = free_list;
    new_block->prev = NULL;
    free_list = new_block;
  }
}
/*
* Function that removes a block from
* the free list */
static void remove_from_free_list(void *bp) {
  // type cast the block pointer to a list node
  list_node *allocated_block = (list_node *)(bp);
  list_node *allocated_next = allocated_block->next;
  list_node *allocated_prev = allocated_block->prev;

  // CASE 1: PREVIOUS NULL, NEXT NULL
  if (allocated_prev == NULL && allocated_next == NULL) {
    allocated_block = NULL;
  }
  // CASE 2: PREVIOUS NULL, NEXT NOT NULL
  else if (allocated_prev == NULL && allocated_next != NULL) {
    allocated_block = allocated_next;
    allocated_block->prev = NULL;
  }
  // CASE 3: PREVIOUS NOT NULL, NEXT NOT NULL
  else if (allocated_prev != NULL && allocated_next != NULL) {
    allocated_prev->next = allocated_next;
    allocated_next->prev = allocated_prev;
  }
  // CASE 4: PREVIOUS NOT NULL, NEXT NULL
  else {
    allocated_prev->next = NULL;
  }
}

// /* Set a block to allocated
// *  Update block headers/footers as needed
// *  Update free list if applicable
// *  Split block if applicable
// */
// static void set_allocated(void *b, size_t size) {
//   // cast void *b to header block to get old size
//   header_block *current_header = (header_block *)(b);

//   // get the current size in the header
//   size_t old_size = current_header->block_size;

//   // compute a new size
//   size_t size_difference = old_size - size;

// // check if we can split page 
//   if (size_difference >= (sizeof(header_block) + sizeof(footer_block)))

//   // update the current header size
//   current_header->block_size = size_difference;
//   // set the block to allocated
//   current_header->allocated = 1;
//   // create a new footer block
//   footer_block *new_footer = (footer_block *)(b + sizeof(header_block) + size)
//   // 

// }

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}
