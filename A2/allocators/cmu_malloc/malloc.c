/* $Id$ */

/*
 *
 *  CSC 469 - Assignment 2
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "memlib.h"
#include "malloc.h"

name_t myname = {
     /* team name to be displayed on webpage */
     "Smokey",
     /* Full name of first team member */
     "Angela Demke Brown",
     /* Email address of first team member */
     "demke@cs.toronto.edu",
     /* Full name of second team member */
     0,
     /* Email address of second team member */
     0
};


typedef struct free_hdr_s {
  int size;
  struct free_hdr_s *prev;
  struct free_hdr_s *next;
} free_hdr_t;

#define GET(p)       (*(int *)(p))
#define PUT(p, val)  (*(int *)(p) = (val))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#if 0
void print_heap() {
  void *addr;

  printf("Prolog: freep = 0x%x, %d, %d\n",*(free_hdr_t **)dseg_lo, *(int *)(dseg_lo+4), *(int *)(dseg_lo+8));
  
  addr = (void *)(dseg_lo+12);
  while(addr < (dseg_hi)) {
    int bsize = *(int *)addr;
    int rsize = bsize & ~0x7;
    int alloc = bsize & 1;
    if (alloc)
      printf("A");
    else printf("F");
    printf("%d",rsize);
  
    if (*(int *)(addr + rsize) != bsize) {
      printf("X");
    }
    printf(" ");
    addr += rsize;
    if(rsize = 0) break;
  }

  printf("\nEpilog: dseg_hi = 0x%x, last blk = %d\n\n",dseg_hi, *(int *)(dseg_hi - 3));
}

#endif

int mm_init (void)
{
  free_hdr_t *freeblk;
  char *old;
  int size;
  int *ftr;
  free_hdr_t **freelist;

  if(dseg_hi <= dseg_lo) {
    mem_init();
    old = mem_sbrk(mem_pagesize());
    if(old == NULL) {
      fprintf(stderr,"Failed to initialize heap, bailing out\n");
      exit(1);
    }
  }

  freelist = (free_hdr_t **)dseg_lo;
  size = dseg_hi - dseg_lo + 1
;


  /* First word of space is a pointer to the start of the freelist */
  /* Next comes a "magic" prolog block of size 2*sizeof(int) that is 
   * always allocated 
   */
  /* Last word of space is a "magic" epilog block that is always allocated */
  /* total wasted space = 1 pointer + 3 ints */


  /* 1. Set up the free block */
  size_t heap_metadata_size = (sizeof(void *) + 2*sizeof(int) + 7)/8 * 8;
  freeblk = (free_hdr_t *)(dseg_lo+heap_metadata_size);
  freeblk->size = size - sizeof(void *) - 3*sizeof(int);
  freeblk->next = freeblk->prev = NULL;

  ftr = (int *)((void *)freeblk + freeblk->size - sizeof(int));
  *ftr = freeblk->size;
  
  /* 2. Set up the prolog */
  *freelist = freeblk;
  /* size = 2*sizeof(int), allocated = 1 */
  *(int *)(dseg_lo + sizeof(void *)) = 2*sizeof(int) | 1; 
  *(int *)(dseg_lo + sizeof(void *) + sizeof(int)) = 2*sizeof(int) | 1; 

  /* 3. Set up the epilog */
  *(int *)(dseg_lo + size - sizeof(int)) = 1; /* size 0, allocated = 1 */



  return 0;
}

void *expand_heap(size_t amount)
{
  void *new_space = mem_sbrk(amount);

  if(new_space == NULL) {
    fprintf(stderr,"Unable to grow heap by %zu\n",amount);
    exit(1);
  }

  /* "consume" previous epilog block into the new free block */
  free_hdr_t *new_free = (free_hdr_t *)(new_space - sizeof(int));
  assert (new_free->size = 1);

  int *ftr;

  /* check if new block can be coalesced with previous */
  ftr = (int *)new_free - 1;
  if(!(*ftr & 1)) {
    free_hdr_t *prev = (free_hdr_t *)((void*)new_free - *ftr);
    prev->size += amount;
    *(int *)((void *)prev + prev->size - sizeof(int)) = prev->size;
    new_free = prev;
  } else {
    /* initialize the new free block and link it at the head of the list */
    new_free->size = amount;
    new_free->prev = NULL;
    new_free->next = *(free_hdr_t **)dseg_lo;
    if (new_free->next)
      new_free->next->prev = new_free;
    *(free_hdr_t **)dseg_lo = new_free;

    ftr = (int *)((char *)new_free + amount - sizeof(int));
    *ftr = amount;
  }

  /* write the new epilog block */
  *(int *)(new_space + amount - sizeof(int)) = 1;

  assert((ptrdiff_t)(new_space + amount - 1) == (ptrdiff_t)dseg_hi);

  return (void *)new_free;
}

pthread_mutex_t malloc_lock = PTHREAD_MUTEX_INITIALIZER;

void *mm_malloc (size_t size)
{
  pthread_mutex_lock(&malloc_lock);

  free_hdr_t *freelist = *((free_hdr_t **)dseg_lo);
  free_hdr_t *blk = freelist;
  int *ftr;

  /* Take size, add on header and footer overhead and round up
   * to multiple of 8
   */
  size += 2*sizeof(int); /* For hdr and ftr when allocated */

  /* need to be at least large enough to hold free header and footer
   * so we can put the block back on the free list later.
   */
  if (size < (sizeof(free_hdr_t) + sizeof(int))) 
    size = sizeof(free_hdr_t) + sizeof(int);

  /* And now make sure size is a multiple of 8 */
  size = (size + 7) / 8 * 8;

  /* Scan free list looking for block that is large enough */
  /* First fit */
  while(blk && blk->size < size) {
    blk = blk->next;
  }

  if (!blk) {
    int pg_sz = mem_pagesize();
    int amt = (size + pg_sz - 1)/pg_sz * pg_sz;
    blk = (free_hdr_t *)expand_heap(amt);
  }

  /* found a block that's big enough */
  int extra = blk->size - size;
  ftr = (int *)((void *)blk + blk->size - sizeof(int));

  assert(*ftr == blk->size);

  if(extra < sizeof(free_hdr_t)) {

    /* leftover piece isn't large enough, allocate whole thing */
    if(blk->prev) {
      blk->prev->next = blk->next;
    } else {
      /* If blk has no predecessor, it must be the first on the list */
      *(free_hdr_t **)dseg_lo = blk->next;
    }
    if(blk->next) {
      blk->next->prev = blk->prev;
    }
    /* set allocated bit in footer */
    *ftr |= 1;
    /* set allocated bit in header */
    blk->size |= 1;
  } else {
    /* Allocate the "back" of the block, leave initial portion on list */
    free_hdr_t *old = blk;
    ftr = (int *)((char *)blk + extra - sizeof(int));
    blk = (free_hdr_t *)(ftr + 1);
    old->size = extra;
    *ftr = extra;
    assert(*ftr == old->size);
    blk->size = size | 1; /* set allocated bit in header */
    ftr = (int *)((char *)blk + size - sizeof(int));
    *ftr = size | 1;  /* set allocated bit in footer */
    assert(*ftr == blk->size);
  }

  
  /* whew.  Now have a block and extracted from free list */
  void *result = (void *)((int *)blk+1);
  pthread_mutex_unlock(&malloc_lock);
  return result;

}

void mm_free (void *ptr)
{
  pthread_mutex_lock(&malloc_lock);
  free_hdr_t *new_free = (free_hdr_t *)(ptr - sizeof(int));
  free_hdr_t **freelist = (free_hdr_t **)dseg_lo;
  new_free->size &= ~0x7;  /* clear allocated bit in header */
  int prev_alloc = *(int *)((void *)new_free - sizeof(int)) & 0x1;
  int next_alloc = *(int *)((void *)new_free + new_free->size) & 0x1;
  int *ftr;  /* Used only in assert() for debugging coalescing code. */
  (void)ftr; /* Keep gcc quiet in non-debug compilations */

  /* Check for coalescing. */

  /* If the next block is free, splice it out of the list and
   * increase the size of the new block 
   */
  if( !next_alloc ) {
    free_hdr_t *next_blk = (free_hdr_t *)((void *)new_free + new_free->size);
    ftr = (int *)((void *)next_blk + next_blk->size - sizeof(int));
    new_free->size += next_blk->size;
    *(int *)((void *)new_free + new_free->size - sizeof(int)) = new_free->size;
    if(next_blk->prev) {
      next_blk->prev->next = next_blk->next;
    } else {
      *(freelist) = next_blk->next;
    }
    if(next_blk->next)
      next_blk->next->prev = next_blk->prev;
    next_blk->prev = next_blk->next = NULL;
    assert(*ftr == new_free->size);
  }

  /* If the previous block is free, just increase its size */
  if( !prev_alloc ) {
    int prev_size = *(int *)((void *)new_free - sizeof(int));
    free_hdr_t *prev_blk = (free_hdr_t *)((void *)new_free - prev_size);
    prev_blk->size += new_free->size;
    *(int *)((void *)prev_blk + prev_blk->size - sizeof(int)) = prev_blk->size;

  } else {
    *(int *)((void *)new_free + new_free->size - sizeof(int)) = new_free->size;
    /* put the new block on the head of the free list */
    new_free->next = *freelist;
    new_free->prev = NULL;
    if(new_free->next)
      new_free->next->prev = new_free;
    *freelist = new_free;
  }

  pthread_mutex_unlock(&malloc_lock);
}

