/*
 * CSC469 - Parallel Memory Allocator
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 * Simon Scott <simontupperscott@gmail.com>
 */

#include <stdlib.h>
#include <pthread.h>

#include "memlib.h"

//
// Hoard parameters
//
#define ALLOC_HOARD_EMPTY_FRACTION  0.25
#define ALLOC_HOARD_FULLNESS_GROUPS 4
#define ALLOC_HOARD_SUPERBLOCK_SIZE (mem_pagesize())
#define ALLOC_HOARD_SIZE_CLASS_BASE 2
#define ALLOC_HOARD_HEAP_CPU_FACTOR 2

//
// Hoard structures
//

// Stack type
typedef struct {
    size_t  count;
    size_t  capacity;
    size_t* blocks;
} free_list_t;


// Superblock structure
typedef struct SUPERBLOCK_T {
    struct SUPERBLOCK_T* prev;
    struct SUPERBLOCK_T* next;    
    size_t size_class;
    size_t next_block;
    free_list_t free_list;
} superblock_t;

// Heap
typedef struct {
    pthread_mutex_t lock;
    size_t mem_used;
    size_t mem_allocated;
    superblock_t* groups[ALLOC_HOARD_FULLNESS_GROUPS];
} heap_t;

// Allocator context
typedef struct {
    int    heap_count;
    heap_t heap_table[1];
} context_t;

// Thread hashing function
int thread_hash(context_t* context, int identifier) {
    return identifier % context->heap_count;
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

