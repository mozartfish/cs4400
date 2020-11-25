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

#define PAGE_OVERHEAD 48
/**********************************************************************************************/
/* HELPER FUNCTIONS */
static void extend(size_t s);
// static void add_to_free_list(void *bp);
// static void remove_from_free_list(void *bp);
// static void set_allocated(void *bp, size_t size);
// static void *coalesce(void *bp);
/*********************************************************************************************/
/*MACRO CONSTANTS*/
// This assumes you have a struct or typedef called "block_header" and "block_footer"
#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))

// #define PAGE_OVERHEAD 32 // padding + prolog header + prolog footer + epilog header

// Given a payload pointer, get the header or footer pointer
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

/********************************************************************************************/
/*DEFINE DATA STRUCTURES FOR MALLOC */
typedef size_t block_header;
typedef size_t block_footer;

/*Doubly Linked List Data Structure for keeping track of the free blocks */
typedef struct page_chunk_node{
  struct page_chunk_node *next;
} page_chunk_node;
/*********************************************************************************************/
/* GLOBAL VARIABLES */
static page_chunk_node *first_page_chunk= NULL; // global variable for keeping track of the free blocks
static void *first_bp = NULL;
// void *current_avail = NULL;         // global variable for keeping of the memory returned by mem_map
// int current_avail_size = 0;         // the size of the available pages
/**********************************************************************************************/
/* 
 * mm_init - initialize the malloc package.
 */

int mm_init(void)
{
  // initialize structs
  first_page_chunk = NULL;
  first_bp = NULL;
  // current_avail = NULL;
  // current_avail_size = 0;
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  int new_size = ALIGN(size + PAGE_OVERHEAD);

  // check if the heap is empty
  if (first_page_chunk == NULL) {
    // call the extend function
  }

  // // traverse the free list looking for space
  // // create a new list node variable that starts at the free list
  // // since the free list gets updated
  // list_node *start = free_list;

  // // look for space continously
  //   // while there is an available free block in the free list
  //   while (start != NULL)
  //   {
  //     // get the header for the current free block in the list
  //     char *header = HDRP(start);

  //     // get the size in the header
  //     size_t available_size = GET_SIZE(header);

  //     if (available_size >= newsize)
  //     {
  //       // allocate the space
  //       set_allocated(start, available_size);
  //       p = (void *)(start);
  //       return p;
  //     }
  //     // check if the next free block is null
  //     if (start->next == NULL) {
  //       extend(newsize);
  //       start = free_list;
  //     }
  //     else {
  //       start = start->next;
  //     }
  //   }
  //   return NULL;
}

/* Request more memory by calling mem_map
*  Initialize the new chunk of memory as applicable
*  Update free list if applicable
*/
static void extend(size_t s)
{
  // align the number of bytes for the pages requested
  size_t total_bytes = PAGE_ALIGN(s);

  // request some memory
  void *raw_bytes = mem_map(total_bytes); // currently a void * pointer

  // Constants for the different parts of the page list chunk
  void *page_info = raw_bytes;
  void *prologue_header = raw_bytes + sizeof(page_chunk_node);
  void *prologue_footer = raw_bytes + sizeof(page_chunk_node) + sizeof(block_header);
  void *payload_header = raw_bytes + sizeof(page_chunk_node) + sizeof(block_header) + sizeof(block_footer);
  void *payload = raw_bytes + sizeof(page_chunk_node) + sizeof(block_header) + sizeof(block_footer) + sizeof(block_header);
  void *payload_footer = FTRP(payload);
  void *epilogue_header = payload_footer + sizeof(block_footer);

  // Padding contains some unused value according to textbook
  PUT(prologue_header, PACK(OVERHEAD, 1));
  PUT(prologue_footer, PACK(OVERHEAD, 1));
  PUT(payload_header, PACK(total_bytes - PAGE_OVERHEAD, 0));
  PUT(payload_footer, PACK(total_bytes - PAGE_OVERHEAD, 0));
  PUT(epilogue_header, PACK(0, 1));

  // add the node to the free block linked list
  add_to_implicit_list(payload_pointer);
}

/*
* Function that takes a block
* and adds it to the free list
*/
static void add_to_implicit_list(void *bp)
{
  // cast the block pointer to a node pointer
  list_node *new_block = (list_node *)(bp);

  // CASE 1: THE FREE LIST IS EMPTY
  if (implicit_free_list == NULL)
  {
    implicit_free_list = new_block;
    implicit_free_list->next = NULL;
    implicit_free_list->prev = NULL;
  }
  // CASE 2: FREE BLOCK LIST IS NOT EMPTY
  else
  {
    implicit_free_list->prev = new_block;
    new_block->next = implicit_free_list;
    new_block->prev = NULL;
    implicit_free_list = new_block;
  }
}
// /*
// * Function that removes a block from
// * the free list */
// static void remove_from_free_list(void *bp)
// {
//   // type cast the block pointer to a list node
//   list_node *allocated_block = (list_node *)(bp);
//   list_node *allocated_next = allocated_block->next;
//   list_node *allocated_prev = allocated_block->prev;

//   // CASE 1: PREVIOUS NULL, NEXT NULL
//   if (allocated_prev == NULL && allocated_next == NULL)
//   {
//     allocated_block = NULL;
//   }
//   // CASE 2: PREVIOUS NULL, NEXT NOT NULL
//   else if (allocated_prev == NULL && allocated_next != NULL)
//   {
//     allocated_block = allocated_next;
//     allocated_block->prev = NULL;
//   }
//   // CASE 3: PREVIOUS NOT NULL, NEXT NOT NULL
//   else if (allocated_prev != NULL && allocated_next != NULL)
//   {
//     allocated_prev->next = allocated_next;
//     allocated_next->prev = allocated_prev;
//   }
//   // CASE 4: PREVIOUS NOT NULL, NEXT NULL
//   else
//   {
//     allocated_prev->next = NULL;
//   }
// }

// /* Set a block to allocated
// *  Update block headers/footers as needed
// *  Update free list if applicable
// *  Split block if applicable
// */

// static void set_allocated(void *bp, size_t size)
// {
//   // get the current header given the the payload pointer
//   char *block_header = HDRP(bp);
//   char *block_footer = FTRP(bp);

//   // get the size from the header
//   size_t old_size = GET_SIZE(block_header);

//   // update the block header and footer
//   PUT(block_header, PACK(size, 1));
//   PUT(block_footer, PACK(size, 1));

//   // remove the block from the free list
//   remove_from_free_list(bp);

//   // check if we should split a page
//   // this means we can fit more than the page overhead
//   size_t size_difference = old_size - size;
//   if (size_difference > PAGE_OVERHEAD)
//   {
//     // GET THE NEXT BLOCK PAYLOAD
//     char *next_payload = NEXT_BLKP(bp);

//     // UPDATE THE HEADER/FOOTER INFO ABOUT THE BLOCK Payload
//     PUT(HDRP(next_payload), PACK(size_difference, 0));
//     PUT(FTRP(next_payload), PACK(size_difference, 0));

//     // add the new payload pointer to the free list
//     add_to_free_list(next_payload);
//   }
//   else
//   {
//     // allocate the entire block of space
//     PUT(block_header, PACK(old_size, 1));
//     PUT(block_footer, PACK(old_size, 1));
//   }
// }
/* Coalesce a free block if applicable*  Returns pointer to new coalesced block*/
// static void *coalesce(void *bp)
// {
// }

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  size_t size = GET_SIZE(HDRP(ptr));
  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));

  // coalesce goes here
}
