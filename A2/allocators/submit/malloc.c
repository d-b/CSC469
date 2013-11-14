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
#include <sys/syscall.h>
#include <sys/types.h>

#include "memlib.h"
#include "mm_thread.h"

//
// Architecture parameters
//
#define ARCH_CACHE_ALIGNMENT 64

//
// Hoard parameters
//
#define ALLOC_HOARD_FULLNESS_GROUPS 4
#define ALLOC_HOARD_MIN_SUPERBLOCKS 1
#define ALLOC_HOARD_SIZE_CLASS_BASE 2
#define ALLOC_HOARD_SIZE_CLASS_MIN  2
#define ALLOC_HOARD_HEAP_CPU_FACTOR 1

//
// Hoard structures
//

// Forward declarations
struct SUPERBLOCK_T;
struct HEAP_T;
struct CONTEXT_T;

// Types
typedef struct SUPERBLOCK_T superblock_t;
typedef struct HEAP_T heap_t;
typedef struct CONTEXT_T context_t;

// Invalid block
enum E_BLOCK { BLOCK_INVALID = -1 };

// Block pointer type
typedef int32_t blockptr_t;

// Superblock structure
struct SUPERBLOCK_T {
    heap_t*         heap;
    uint8_t         group;
    int             size_class;
    size_t          block_size;
    size_t          block_count;
    size_t          block_used;
    blockptr_t      next_block;
    blockptr_t      next_free;
    superblock_t*   prev;
    superblock_t*   next;
} __attribute__((aligned(ARCH_CACHE_ALIGNMENT)));

// Heap
struct HEAP_T {
    pthread_mutex_t lock;
    int             index;
    size_t          mem_used;
    size_t          mem_allocated;
    superblock_t*   bins[ALLOC_HOARD_FULLNESS_GROUPS];
};

// Allocator context
struct CONTEXT_T {
    void*           blocks_base;
    int             heap_count;
    heap_t          heap_table[1];
};

//
// Utility functions
//

size_t          util_system_pagesize;
pthread_mutex_t util_allocator_lock;

static void util_init(void) {
    if(dseg_hi <= dseg_lo) mem_init();
    pthread_mutex_init(&util_allocator_lock, NULL);
    util_system_pagesize = mem_pagesize();
}

inline pid_t util_gettid(void) {
    return (pid_t) syscall(SYS_gettid);
}

inline void* util_allocate(size_t size) {
    pthread_mutex_lock(&util_allocator_lock);
    void* mem = mem_sbrk(size);
    pthread_mutex_unlock(&util_allocator_lock);
    return mem;
}

inline void* util_desg_lo() {
    return dseg_lo;
}

inline void* util_desg_hi() {
    return dseg_hi;
}

inline size_t util_pagesize() {
    return util_system_pagesize;
}

inline size_t util_pagealigned(size_t size) {
    size_t page_size = util_pagesize();
    return ((size + page_size - 1)/page_size)*page_size;
}

inline int util_sizeclass(size_t size) {
    // Compute size class
    size_t size_unit = 1; size_t x = size;
    int size_class = 0; while(x >= ALLOC_HOARD_SIZE_CLASS_BASE) {
        x          /= ALLOC_HOARD_SIZE_CLASS_BASE;
        size_unit  *= ALLOC_HOARD_SIZE_CLASS_BASE;
        size_class += 1;
    }

    // Check for remainder and return size class
    if(size % size_unit) size_class += 1;
    return (size_class < ALLOC_HOARD_SIZE_CLASS_MIN) ? ALLOC_HOARD_SIZE_CLASS_MIN : size_class;
}

//
// Superblock functions
//

inline size_t superblock_footprint(void) {
    return util_pagesize() + sizeof(superblock_t);
}

inline size_t superblock_size(void) {
    return util_pagesize();
}

static void superblock_link(superblock_t* sb, heap_t* heap, int group) {
    // Set new heap and group
    sb->heap = heap;
    sb->group = group;

    // Perform standard link operation
    if(sb->heap->bins[sb->group])
        sb->heap->bins[sb->group]->prev = sb;
    sb->next = sb->heap->bins[sb->group];
    sb->heap->bins[sb->group] = sb;
}

static void superblock_unlink(superblock_t* sb) {
    // See if we were the head
    if(sb->heap->bins[sb->group] == sb)
        sb->heap->bins[sb->group] = sb->next;

    // Perform standard unlink operation
    if(sb->prev) sb->prev->next = sb->next;
    if(sb->next) sb->next->prev = sb->prev;
    sb->prev = sb->next = NULL;
}

static void superblock_transform(superblock_t* sb, int size_class) {
    // Set size class
    sb->size_class = size_class;

    // Compute block size
    sb->block_size = 1;
    int i; for(i = 0; i < sb->size_class; i++)
        sb->block_size *= ALLOC_HOARD_SIZE_CLASS_BASE;

    // Set initials
    sb->group       = 0;
    sb->block_used  = 0;
    sb->block_count = superblock_size() / sb->block_size;
    sb->next_block  = 0;
    sb->next_free   = BLOCK_INVALID;
}

static void superblock_init(superblock_t* sb, heap_t* heap, int size_class) {
    // Transform the superblock into the specified size class
    superblock_transform(sb, size_class);

    // Set initials
    sb->prev = NULL;
    sb->next = NULL;

    // Perform link
    superblock_link(sb, heap, 0);

    // Update heap statistics
    sb->heap->mem_allocated += sb->block_count * sb->block_size;
}

static superblock_t* superblock_allocate(heap_t* heap, int size_class) {
    // Try to allocate a superblock
    superblock_t* sb = (superblock_t*) util_allocate(superblock_footprint());
    if(!sb) return NULL;

    // Initialize the superblock and return it
    superblock_init(sb, heap, size_class);
    return sb;
}

static int superblock_group(superblock_t* sb) {
    size_t group_size = sb->block_count / ALLOC_HOARD_FULLNESS_GROUPS;
    if(!group_size) group_size = sb->block_count / 2;
    int group = sb->block_used / group_size;
    return (group >= ALLOC_HOARD_FULLNESS_GROUPS) ?
        ALLOC_HOARD_FULLNESS_GROUPS - 1 : group;
}

static void superblock_move(superblock_t* sb) {
    int group = superblock_group(sb);
    if(sb->heap->bins[group] != sb) {
        superblock_unlink(sb);
        superblock_link(sb, sb->heap, group);
    }
}

static void superblock_transfer(superblock_t* sb, heap_t* heap) {
    // See if we need to transfer
    if(sb->heap == heap) return;

    // Compute stats
    size_t used = sb->block_used * sb->block_size;
    size_t allocated = sb->block_count * sb->block_size;

    // Unlink from current heap
    superblock_unlink(sb);
    sb->heap->mem_used -= used;
    sb->heap->mem_allocated -= allocated;
    
    // Link to new heap
    heap->mem_used += used;
    heap->mem_allocated += allocated;
    superblock_link(sb, heap, sb->group);
}

inline blockptr_t superblock_block_find(superblock_t* sb, void* ptr) {
    return (blockptr_t) ((((char*) ptr - (char*) sb) - sizeof(superblock_t)) / sb->block_size);
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

    // Update heap statistics & move block
    sb->heap->mem_used += sb->block_size;
    superblock_move(sb);

    // Return a new or freed block
    blockptr_t blk = superblock_freelist_pop(sb);
    return (blk != BLOCK_INVALID) ? blk : sb->next_block++;
}

static void superblock_block_free(superblock_t* sb, blockptr_t blk) {
    // Add block to free list and adjust stats
    assert(sb->block_used > 0);
    superblock_freelist_push(sb, blk);
    sb->block_used -= 1;

    // Update heap statistics & move
    sb->heap->mem_used -= sb->block_size;
    superblock_move(sb);
}

static heap_t* superblock_heap_lock(volatile superblock_t* sb) {
    // The heap
    heap_t* heap;

    // Heap locking functions
    void heap_lock(heap_t* heap);
    void heap_unlock(heap_t* heap);

    // Try to acquire the heap lock
    for(;;) {
        // Get the superblock's current heap
        heap = sb->heap;

        // Lock the heap
        heap_lock(heap);

        // Return the heap if it has not changed
        if(heap == sb->heap) return heap;

        // Unlock the heap and try again
        heap_unlock(heap);
    }
}

//
// Heap functions
//

inline void heap_lock(heap_t* heap) {
    pthread_mutex_lock(&heap->lock);
}

inline void heap_unlock(heap_t* heap) {
    pthread_mutex_unlock(&heap->lock);
}

inline int heap_is_fluid(heap_t* heap) {
    return (heap->mem_allocated - heap->mem_used) > (superblock_size() * ALLOC_HOARD_MIN_SUPERBLOCKS);
}

static void heap_init(heap_t* heap) {
    pthread_mutex_init(&heap->lock, NULL);
    heap->mem_used = 0;
    heap->mem_allocated = 0;
    int i; for(i = 0; i < ALLOC_HOARD_FULLNESS_GROUPS; i++)
        heap->bins[i] = NULL;
}

static superblock_t* heap_scan(heap_t* heap, int size_class) {
    // Scan heap for a superblock of the given size class
    superblock_t* sb = NULL; int group;
    for(group = ALLOC_HOARD_FULLNESS_GROUPS - 1; group >= 0; group--) {
        for(sb = heap->bins[group]; sb; sb = sb->next)
            if(sb->size_class == size_class &&
               sb->block_used <  sb->block_count) break;
        // If a superblock was found break out
        if(sb) break;
    } return sb;
}

//
// Context functions
//

inline size_t context_size() {
    size_t size = sizeof(context_t) + sizeof(heap_t) * getNumProcessors() * ALLOC_HOARD_HEAP_CPU_FACTOR;
    return util_pagealigned(size);
}

static void context_init(context_t* ctx) {
    ctx->blocks_base = (char*) ctx + context_size();
    ctx->heap_count = getNumProcessors() * ALLOC_HOARD_HEAP_CPU_FACTOR;
    int i; for(i = 0; i <= ctx->heap_count; i++) {
        heap_init(&ctx->heap_table[i]);
        ctx->heap_table[i].index = i;
    }
}

static heap_t* context_globalheap(context_t* ctx) {
    return &ctx->heap_table[0];
}

static heap_t* context_heap(context_t* ctx, uint32_t threadid) {
    return &ctx->heap_table[1 + (threadid % ctx->heap_count)];
}

static superblock_t* context_superblock_find(context_t* ctx, void* ptr) {
    size_t size = superblock_footprint();
    return (superblock_t*) ((char*) ctx->blocks_base + ((ptr - ctx->blocks_base) / size) * size);
}

static void* context_malloc(context_t* ctx, size_t sz) {
    // Get the local heap and lock it
    heap_t* heap = context_heap(ctx, util_gettid());
    heap_lock(heap);

    // Allocated memory
    void* mem = NULL;

    // Compute size class
    int size_class = util_sizeclass(sz);

    // Attempt to find an appropriate superblock in the local heap
    superblock_t* sb = heap_scan(heap, size_class);

    // If no superblock was found
    if(!sb) {
        // Get and lock global heap
        heap_t* glob = context_globalheap(ctx);
        heap_lock(glob);

        // Try to find a suitable superblock on the global heap
        for(sb = glob->bins[0]; sb; sb = sb->next) {
            // If the superblock is empty
            if(!sb->block_used) {
                superblock_transform(sb, size_class); break;
            }
            // If the superblock matches requested size class
            else if(sb->size_class == size_class) break;
        }

        // Transfer superblock to local heap
        if(sb) superblock_transfer(sb, heap);
        // If no superblock was found try to allocate a new one
        else sb = superblock_allocate(heap, size_class);

        // Unlock global heap
        heap_unlock(glob);
    }

    // Finally, allocate the block unless we are out of memory
    if(sb) {
        blockptr_t blk = superblock_block_allocate(sb);
        mem = superblock_block_data(sb, blk);
    }

    // Unlock local heap and return memory
    heap_unlock(heap);
    return mem;
}

static void context_free(context_t* ctx, void* ptr) {
    // Find superblock
    superblock_t* sb = context_superblock_find(ctx, ptr);
    heap_t* heap = superblock_heap_lock(sb);

    // Find and free block
    blockptr_t blk = superblock_block_find(sb, ptr);
    superblock_block_free(sb, blk);

    // If the superblock is mostly empty and the heap is fluid
    if(sb->heap->index != 0 && sb->group == 0 && heap_is_fluid(sb->heap)) {
        // Get and lock global heap
        heap_t* glob = context_globalheap(ctx);
        heap_lock(glob);

        // Perform the transfer
        superblock_transfer(sb, glob);

        // Unlock the global heap
        heap_unlock(glob);
    }

    // Unlock the heap
    heap_unlock(heap);
}

//
// Global context
//

inline context_t* get_context(void) {
    return (context_t*) util_desg_lo();
}

//
// Library
//

void *mm_malloc(size_t sz)
{
    // See if we are allocating from system
    if(sz > superblock_size()/2)
        return malloc(sz);

    // Allocate and return the memory
    return context_malloc(get_context(), sz);
}

void mm_free(void *ptr)
{
    // See if it was a system allocation
    if(ptr < util_desg_lo() || ptr >= util_desg_hi()) {
        free(ptr); return;
    }

    // Free the object
    context_free(get_context(), ptr);
}

int mm_init(void)
{
    // Initialize utility functions
    util_init();

    // Allocate memory for the context and initialize it
    context_t* ctx = util_allocate(context_size());
    if(ctx == NULL) return -1;
    context_init(ctx);
    return 0;
}
