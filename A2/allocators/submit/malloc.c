/*
 * CSC469 - Parallel Memory Allocator
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 * Simon Scott <simontupperscott@gmail.com>
 */

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "memlib.h"

//
// Hoard parameters
//
#define ALLOC_HOARD_EMPTY_FRACTION  0.25
#define ALLOC_HOARD_FULLNESS_GROUPS 4
#define ALLOC_HOARD_SIZE_CLASS_BASE 2
#define ALLOC_HOARD_SIZE_CLASS_MIN  2
#define ALLOC_HOARD_HEAP_CPU_FACTOR 2

//
// Hoard structures
//

// Invalid block
enum E_BLOCK { BLOCK_INVALID = -1 };

// Block pointer type
typedef int32_t blockptr_t;

// Superblock structure
typedef struct SUPERBLOCK_T {
    int size_class;
    size_t block_size;
    size_t block_count;
    size_t block_used;
    blockptr_t next_block;
    blockptr_t next_free;
    struct SUPERBLOCK_T* prev;
    struct SUPERBLOCK_T* next;
} superblock_t;

// Heap
typedef struct {
    pthread_mutex_t lock;
    size_t mem_used;
    size_t mem_allocated;
    superblock_t* bins[ALLOC_HOARD_FULLNESS_GROUPS];
} heap_t;

// Allocator context
typedef struct {
    int    heap_count;
    heap_t heap_table[1];
} context_t;

//
// Superblock functions
//

inline size_t superblock_size(void) {
    return mem_pagesize() - sizeof(superblock_t);
}

static void superblock_init(superblock_t* sb, int size_class) {
    // Set size class
    sb->size_class = size_class;

    // Compute block size
    sb->block_size = 1;
    int i; for(i = 0; i < sb->size_class; i++)
        sb->block_size *= ALLOC_HOARD_SIZE_CLASS_BASE;

    // Set initials
    sb->block_used  = 0;
    sb->block_count = superblock_size() / sb->block_size;
    sb->next_block  = 0;
    sb->next_free   = BLOCK_INVALID;
    
    // Link entries
    sb->next = NULL;
    sb->prev = NULL;
}

inline void* superblock_block_data(superblock_t* sb, blockptr_t blk) {
    assert(blk < sb->block_count);
    return (void*) ((char*) sb + sizeof(superblock_t) + sb->block_size * blk);
}

static void superblock_freelist_push(superblock_t* sb, blockptr_t blk) {
    // Embed a pointer to the previous block
    blockptr_t* prev_free = (blockptr_t*) superblock_block_data(sb, blk);
    *prev_free = sb->next_free;

    // Set our new next free block
    sb->next_free = blk;
}

static blockptr_t superblock_freelist_pop(superblock_t* sb) {
    // See if there are no free blocks
    if(sb->next_free == BLOCK_INVALID)
        return BLOCK_INVALID;

    // Pop the block off the stack
    blockptr_t blk = sb->next_free;
    sb->next_free = *(blockptr_t*) superblock_block_data(sb, blk);
    return blk;
}

static blockptr_t superblock_block_allocate(superblock_t* sb) {
    // See if we are filled to capacity
    if(sb->block_used >= sb->block_count)
        return BLOCK_INVALID;
    sb->block_used += 1;

    // Return a new or freed block
    blockptr_t blk = superblock_freelist_pop(sb);
    return (blk != BLOCK_INVALID) ? blk : sb->next_block++;
}

static void superblock_block_free(superblock_t* sb, blockptr_t blk) {
    // Add block to free list and adjust stats
    assert(sb->block_used > 0);
    superblock_freelist_push(sb, blk);
    sb->block_used -= 1;
}

void *mm_malloc(size_t sz)
{
	(void)sz; /* Avoid warning about unused variable */
	return NULL;
}

void mm_free(void *ptr)
{
	(void)ptr; /* Avoid warning about unused variable */
}


int mm_init(void)
{
	return 0;
}
