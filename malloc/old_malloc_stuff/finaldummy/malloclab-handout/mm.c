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
 * Implements a implicit linked list 
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
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize() - 1)) & ~(mem_pagesize() - 1))

// This assumes you have a struct or typedef called "block_header" and "block_footer"
#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))

// Given a payload pointer, get the header or footer pointer
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - OVERHEAD)

// Given a payload pointer, get the next or previous payload pointer
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-OVERHEAD))
/*************************************************************************************/
// size_t macros
// Given a pointer to a header, get or set its value
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

// Combine a size and alloc bit
#define PACK(size, alloc) ((size) | (alloc))

// Given a header pointer, get the alloc or size
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_SIZE(p) (GET(p) & ~0xF)

// overall overhead for a page
#define PAGE_OVERHEAD 48 // padding + prolog header + prolog footer + epilog header

// define a word size
#define WSIZE 8  // word/header and footer size (bytes)
#define DSIZE 16 // double word size in bytes
/**************************************************************************************/
/* HELPER FUNCTIONS */
// static void extend(size_t new_size);
// static void set_allocated(void *bp, size_t size);
// static int heap_checker(void *bp);
// static void *coalesce(void *bp);
// static void add_page_chunk(void *memory);
static void add_free_block(void *fbp);

/****************************************************************************************************/

/* GLOBAL VARIABLES AND DATA STRUCTURES */
typedef size_t block_header;
typedef size_t block_footer;

typedef struct free_block
{
  struct free_block *next;
  struct free_block *prev;
} free_block;

// global pointer to the first page chunk
static free_block *first_free_block;

/***************************************************************************************************/

/* 
 * mm_init - initialize the malloc package.
 */
// int mm_init(void)
// {
//   first_bp = NULL;
//   bp = NULL;
//   first_page_chunk = NULL;
//   return 0;
// }

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  // int new_size = ALIGN(size + OVERHEAD);
  // // check if there is a page chunk available
  // if (first_page_chunk == NULL)
  // {
  //   extend(new_size);
  // }

  // // set bp pointer
  // bp = first_bp;

  // while (1)
  // {
  //   while (GET_SIZE(HDRP(bp)) != 0)
  //   {
  //     if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp))) >= new_size)
  //     {
  //       set_allocated(bp, new_size);
  //       return bp;
  //     }
  //       bp = NEXT_BLKP(bp);
  //   }

  //   // if we reach an epilogue check if there is another page chunk
  //   if (first_page_chunk->next == NULL)
  //   {
  //     extend(new_size);
  //     bp = first_bp;
  //   }
  //   else
  //   {
  //     bp = sizeof(page_chunk) + OVERHEAD + sizeof(block_header);
  //   }
  // }
}

/*
 * mm_free - Freeing a block does nothing.
 */
// void mm_free(void *ptr)
// {
//   size_t size = GET_SIZE(HDRP(ptr));
//   PUT(HDRP(bp), PACK(size, 0));
//   PUT(FTRP(ptr), PACK(size, 0));

//   // call the coalesce function
//   coalesce(ptr);
// }

// static void extend(size_t new_size)
// {
//   // get a chunk of pages that satisfy the requested size
//   size_t current_size = PAGE_ALIGN(new_size);

//   // get a number p bytes that are equivalent to page_chunk_size
//   void *p = mem_map(current_size);

//   // add a page chunk to the linked list
//   add_page_chunk(p);

//   p += sizeof(page_chunk);                                    // move the p pointer past the first 16 bytes since that's assigned for the pages
//   PUT(p, 0);                                                  // padding of 8 bytes
//   PUT(p + 8, PACK(OVERHEAD, 1));                              // PROLOGUE Header;
//   PUT(p + 16, PACK(OVERHEAD, 1));                             // PROLOGUE FOOTER;
//   PUT(p + 24, PACK(current_size - PAGE_OVERHEAD, 0));         // Payload Header
//   first_bp = p + 32;                                          // Payload memory
//   PUT(FTRP(first_bp), PACK(current_size - PAGE_OVERHEAD, 0)); // Payload Footer
//   PUT(FTRP(first_bp) + 8, PACK(0, 1));                        // EPILOGUE Header
// }

// static int heap_checker(void *bp)
// {
//   void *p = NULL;

//   // set bp pointer
//   p = bp;

//   while (p != NULL)
//   {
//     printf("The size of the current block is: %d\n", GET_SIZE(HDRP(p)));
//     printf("The current block allocation status is: %d\n", GET_ALLOC(HDRP(p)));
//     if (GET_ALLOC(HDRP(p)) != 1)
//     {
//       printf("The block should be allocated!\n");
//       return -1;
//     }
//     else
//     {
//       printf("The next block should be free\n");
//       // check if the current block is allocated and get its next block
//       p = NEXT_BLKP(p);
//       printf("The size of the current block is: %d\n", GET_SIZE(HDRP(p)));
//       printf("The current block allocation status is: %d\n", GET_ALLOC(HDRP(p)));

//       return 1;

//       // if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp))) >= new_size)
//       // {
//       //   set_allocated(bp, new_size);
//       //   return bp;
//       // }
//       // bp = NEXT_BLKP(bp);
//     }
//   }
//   return 1;
// }

/**
 * This function adds a free block to the free block list
 * using a LIFO Procedure as described in the textbook
 * page 863 */
static void add_free_block(void *fbp)
{
  // cast fbp to a free block node
  free_block *new_free_block = (free_block *)(fbp);

  // CASE 1: THE FIRST FREE BLOCK IS NULL
  if (first_free_block = NULL)
  {
    first_free_block = new_free_block;
    first_free_block->next = NULL;
    first_free_block->prev = NULL;
  }
  // CASE 2: THE FIRST FREE BLOCK IS NOT NULL
  // ADD THE NEW FREE BLOCK TO THE FRONT OF THE LINKED LIST ACCORDING TO LIFO POLICY
  else
  {
    first_free_block->prev = new_free_block;
    new_free_block->next = first_free_block;
    new_free_block->prev = NULL;
    first_free_block = new_free_block;
  }
}

/**
 * This function removes a free block from the free list
 **/
static void remove_free_block(void *abp)
{
  // cast abp to a free block node
  free_block *allocated_block = (free_block *)(abp);

  // CASE 1: allocated block is the first free block (according to LIFO POLICY in textbook)
  if (allocated_block == first_free_block)
  {
    // CASE 1: first free block has zero children
    if (allocated_block->next == NULL && allocated_block->prev == NULL)
    {
      allocated_block->next == NULL;
      allocated_block->prev = NULL;
      allocated_block = NULL;
    }
    // CASE 2: first free block has a next block
    else
    {
      if (allocated_block->next != NULL && allocated_block->prev == NULL)
      {
        first_free_block = allocated_block->next;
        first_free_block->prev = NULL;
      }
    }
  }
  // CASE 2: delete a block in the middle of the linked list of blocks
  else if (allocated_block->next != NULL && allocated_block->prev != NULL)
  {
    free_block *allocated_prev = allocated_block->prev;
    free_block *allocated_next = allocated_block->next;
    allocated_prev->next = allocated_next;
    allocated_next->prev = allocated_prev;
  }
  // CASE 3: delete a block at the end of the linked list of blocks
  else
  {
    free_block *allocated_prev = allocated_block->prev;
    allocated_prev->next = NULL;
  }
}
/**
 * This function follows the textbook practice problem on page 
 * 884 */
static void set_allocated(void *bp, size_t allocated_size)
{
  size_t current_size = GET_SIZE(HDRP(bp));

  if ((current_size - allocated_size) >= 2 * DSIZE)
  {
    PUT(HDRP(bp), PACK(allocated_size, 1));
    PUT(FTRP(bp), PACK(allocated_size, 1));
    // remove the allocated block
    remove_free_block(bp);
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(current_size - allocated_size, 0));
    PUT(FTRP(bp), PACK(current_size - allocated_size, 0));
  }
  else
  {
    PUT(HDRP(bp), PACK(current_size, 1));
    PUT(FTRP(bp), PACK(current_size, 1));
    remove_free_block(bp);
  }
}
// static void add_page_chunk(void *memory)
// {
//   // cast memory to a page chunk
//   page_chunk *new_page_chunk = (page_chunk *)(memory);

//   if (first_page_chunk == NULL)
//   {
//     new_page_chunk->next = NULL;
//     new_page_chunk->prev = NULL;
//     first_page_chunk = new_page_chunk;
//   }
//   else
//   {
//     // set the first page chunk previous
//     first_page_chunk->prev = new_page_chunk;

//     // set the new page chunk next
//     new_page_chunk->next = first_page_chunk;

//     // set the new page chunk previous
//     new_page_chunk->prev = NULL;

//     // set the first page chunk as the next page chunk
//     first_page_chunk = new_page_chunk;
//   }
// }

// static void set_allocated(void *bp, size_t size)
// {
//   // print the next block pointer to make sure it is the epilogue
//   // printf("The epilogue header size is: %d\n", GET_SIZE(HDRP(NEXT_BLKP(bp))));
//   // printf("The epilogue should be allocated: %d\n", GET_ALLOC(HDRP(NEXT_BLKP(bp))));

//   size_t extra_size = GET_SIZE(HDRP(bp)) - size;
//   // Check if we can split the page
//   if (extra_size > ALIGN(PAGE_OVERHEAD))
//   {
//     PUT(HDRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
//     PUT(FTRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
//   }
//   PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
//   PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
// }

// static void *coalesce(void *bp)
// {
//   size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
//   size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
//   size_t size = GET_SIZE(HDRP(bp));

//   // CASE 1: BOTH ALLOCATED
//   if (prev_alloc && next_alloc)
//   {
//     return bp;
//   }

//   // CASE 2: PREVIOUS ALLOCATED and next not allocated
//   else if (prev_alloc && !next_alloc)
//   {
//     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
//     PUT(HDRP(bp), PACK(size, 0));
//     PUT(FTRP(bp), PACK(size, 0));
//   }

//   // CASE 3: previous alloacted and next not allocated
//   else if (!prev_alloc && next_alloc)
//   {
//     size += GET_SIZE(HDRP(PREV_BLKP(bp)));
//     PUT(FTRP(bp), PACK(size, 0));
//     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
//     bp = PREV_BLKP(bp);
//   }

//   // both unallocated
//   else
//   {
//     size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
//     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
//     PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
//     bp = PREV_BLKP(bp);
//   }

//   return bp;
// }
