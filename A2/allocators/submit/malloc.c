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
#define ALLOC_HOARD_SIZE_CLASS_MIN  3
#define ALLOC_HOARD_HEAP_CPU_FACTOR 1

//
// Hoard structures
//

// Forward declarations
struct LARGEBLOCK_T;
struct SUPERBLOCK_T;
struct HEAP_T;
struct CONTEXT_T;

// Types
typedef struct LARGEBLOCK_T largeblock_t;
typedef struct SUPERBLOCK_T superblock_t;
typedef struct ALLOCATION_T allocation_t;
typedef struct HEAP_T heap_t;
typedef struct CONTEXT_T context_t;

// Allocation type
enum E_ALLOCATION { ALLOCATION_NORMAL, ALLOCATION_LARGE };

// Invalid block
enum E_BLOCK { BLOCK_INVALID = -1 };

// Block pointer type
typedef int32_t blockptr_t;

// Largeblock structure
struct LARGEBLOCK_T {
    size_t block_count;
    largeblock_t* prev;
    largeblock_t* next;
};

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
};

// Allocation structure
struct ALLOCATION_T {
    int type;
    union {
        superblock_t sb;
        largeblock_t lb;
    };
} __attribute__((aligned(ARCH_CACHE_ALIGNMENT)));

// Heap
struct HEAP_T {
    pthread_mutex_t lock;
    int             index;
    size_t          mem_used;
    size_t          mem_allocated;
    superblock_t*   bins[1][ALLOC_HOARD_FULLNESS_GROUPS];
};

// Allocator context
struct CONTEXT_T {
    void*           blocks_base;
    pthread_mutex_t largeblock_lock;
    largeblock_t*   largeblock_next;    
    int             heap_count;
    char            heap_table[1];
};

//
// Utility functions
//

size_t          global_util_pagesize;
int             global_util_sizeclasses;
pthread_mutex_t global_util_allocator_lock;

static void util_init(void) {
    int util_sizeclass(size_t size);
    if(dseg_hi <= dseg_lo) mem_init();
    pthread_mutex_init(&global_util_allocator_lock, NULL);
    global_util_pagesize = mem_pagesize();
    global_util_sizeclasses = util_sizeclass(global_util_pagesize >> 2) + 1;
}

inline pid_t util_gettid(void) {
    return (pid_t) syscall(SYS_gettid);
}

inline void* util_allocate(size_t size) {
    pthread_mutex_lock(&global_util_allocator_lock);
    void* mem = mem_sbrk(size);
    pthread_mutex_unlock(&global_util_allocator_lock);
    return mem;
}

inline void* util_desg_lo() {
    return dseg_lo;
}

inline void* util_desg_hi() {
    return dseg_hi;
}

inline size_t util_pagesize() {
    return global_util_pagesize;
}

inline size_t util_cachealigned(size_t size) {
    size_t block_size = ARCH_CACHE_ALIGNMENT;
    return ((size + block_size - 1)/block_size)*block_size;
}

inline int util_sizeclasses() {
    return global_util_sizeclasses;
}

inline int util_sizeclass(size_t size) {
    // Compute size class
    size_t size_unit = 1; size_t x = size;
    int size_class = 0; while(x >= ALLOC_HOARD_SIZE_CLASS_BASE) {
        x          /= ALLOC_HOARD_SIZE_CLASS_BASE;
        size_unit  *= ALLOC_HOARD_SIZE_CLASS_BASE;
        size_class += 1;
    }

    // Check for remainder and increment as needed
    if(size % size_unit) size_class += 1;

    // Enforce minimum and re-base it to be zero indexed
    if(size_class < ALLOC_HOARD_SIZE_CLASS_MIN) size_class = ALLOC_HOARD_SIZE_CLASS_MIN;
    size_class -= ALLOC_HOARD_SIZE_CLASS_MIN;
    return size_class;
}

inline size_t util_sizeclass_size(int size_class) {
    size_t size = 1;
    int i; for(i = 0; i < size_class + ALLOC_HOARD_SIZE_CLASS_MIN; i++)
        size *= ALLOC_HOARD_SIZE_CLASS_BASE;
    return size;
}

inline int util_heapcount() {
    return getNumProcessors() * ALLOC_HOARD_HEAP_CPU_FACTOR;
}

//
// Allocation functions
//

inline size_t allocation_size(void) {
    return util_pagesize() + sizeof(allocation_t);
}


//
// Largeblock functions
//

inline void largeblock_lock(context_t* ctx) {
    pthread_mutex_lock(&ctx->largeblock_lock);
}

inline void largeblock_unlock(context_t* ctx) {
    pthread_mutex_unlock(&ctx->largeblock_lock);
}

inline size_t largeblock_size(largeblock_t* lb) {
    return lb->block_count * allocation_size() - sizeof(largeblock_t);
}

inline void* largeblock_data(largeblock_t* lb) {
    return (void*) ((char*) lb + sizeof(largeblock_t));
}

inline size_t largeblock_blocks(size_t size) {
    if(size == 0) return 0;
    size_t blocks = 1;
    size_t alloc_size = allocation_size();
    size -= allocation_size() - sizeof(largeblock_t);
    if(size > 0) blocks += ((size + alloc_size - 1)/alloc_size);
    return blocks;
}

static void largeblock_link(largeblock_t* lb, context_t* ctx) {
    lb->next = ctx->largeblock_next;
    if(ctx->largeblock_next)
        ctx->largeblock_next->prev = lb;
    ctx->largeblock_next = lb;
}

static void largeblock_unlink(largeblock_t* lb, context_t* ctx) {
    // See if we were the head
    if(ctx->largeblock_next == lb)
        ctx->largeblock_next = lb->next;

    // Perform standard unlink operation
    if(lb->prev) lb->prev->next = lb->next;
    if(lb->next) lb->next->prev = lb->prev;
    lb->prev = lb->next = NULL;
}


static largeblock_t* largeblock_allocate(context_t* ctx, size_t size) {
    // Scan for the smallest suitable free large block
    largeblock_t* min = NULL;
    largeblock_t* lb; for(lb = ctx->largeblock_next; lb; lb = lb->next) {
        if(size <= largeblock_size(lb)) {
            if(!min) min = lb;
            else if(lb->block_count < min->block_count) min = lb;
        }
    }

    // See if a largeblock was found
    if(min) lb = min;

    // If none was found, try to allocate one
    if(!lb) {
        // Allocate the large block
        size_t blocks = largeblock_blocks(size);
        allocation_t* alloc = (allocation_t*) util_allocate(blocks * allocation_size());
        if(!alloc) return NULL;

        // Perform setup
        alloc->type = ALLOCATION_LARGE;
        lb = &alloc->lb;
        lb->block_count = blocks;
        lb->next = NULL;
    }
    // Otherwise remove the found largeblock from the free list
    else largeblock_unlink(lb, ctx);

    // Return the largeblock
    return lb;
}

//
// Superblock functions
//

inline size_t superblock_size(void) {
    return allocation_size() - sizeof(allocation_t);
}

static void superblock_link(superblock_t* sb, heap_t* heap, int group, int size_class) {
    // Set new heap and group
    sb->heap = heap;
    sb->group = group;
    sb->size_class = size_class;

    // Perform standard link operation
    superblock_t* head = sb->heap->bins[size_class][group];
    sb->heap->bins[size_class][group] = sb;
    if(head) head->prev = sb;
    sb->next = head;
}

static void superblock_unlink(superblock_t* sb) {
    // See if we were the head
    if(sb->heap->bins[sb->size_class][sb->group] == sb)
        sb->heap->bins[sb->size_class][sb->group] = sb->next;

    // Perform standard unlink operation
    if(sb->prev) sb->prev->next = sb->next;
    if(sb->next) sb->next->prev = sb->prev;
    sb->prev = sb->next = NULL;
}

static void superblock_transform(superblock_t* sb, int size_class) {
    // Start by unlinking
    superblock_unlink(sb);

    // Set size class
    sb->size_class = size_class;

    // Compute block size
    sb->block_size = util_sizeclass_size(size_class);

    // Set initials
    sb->group       = 0;
    sb->block_used  = 0;
    sb->block_count = superblock_size() / sb->block_size;
    sb->next_block  = 0;
    sb->next_free   = BLOCK_INVALID;

    // Perform the link
    superblock_link(sb, sb->heap, 0, size_class);
}

static void superblock_init(superblock_t* sb, heap_t* heap, int size_class) {
    // Set the initials
    sb->heap = heap;
    sb->prev = NULL;
    sb->next = NULL;    

    // Transform the superblock into the specified size class
    superblock_transform(sb, size_class);

    // Update heap statistics
    sb->heap->mem_allocated += sb->block_count * sb->block_size;
}

static superblock_t* superblock_allocate(heap_t* heap, int size_class) {
    // Try to allocate a superblock
    allocation_t* alloc = (allocation_t*) util_allocate(allocation_size());
    if(!alloc) return NULL;

    // Set the allocation type
    alloc->type = ALLOCATION_NORMAL;

    // Initialize the superblock and return it
    superblock_init(&alloc->sb, heap, size_class);
    return &alloc->sb;
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
    if(sb->heap->bins[sb->size_class][group] != sb) {
        superblock_unlink(sb);
        superblock_link(sb, sb->heap, group, sb->size_class);
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
    superblock_link(sb, heap, sb->group, sb->size_class);
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

inline size_t heap_size() {
    return sizeof(heap_t)
        +  sizeof(superblock_t*) * ALLOC_HOARD_FULLNESS_GROUPS * util_sizeclasses();
}

inline void heap_lock(heap_t* heap) {
    pthread_mutex_lock(&heap->lock);
}

inline void heap_unlock(heap_t* heap) {
    pthread_mutex_unlock(&heap->lock);
}

inline int heap_is_fluid(heap_t* heap) {
    return (heap->mem_allocated - heap->mem_used) > (superblock_size() * ALLOC_HOARD_MIN_SUPERBLOCKS);
}

static void heap_init(heap_t* heap, int index) {
    memset(heap, 0, heap_size());
    pthread_mutex_init(&heap->lock, NULL);
    heap->index = index;
}

static superblock_t* heap_scan(heap_t* heap, int size_class) {
    // Scan heap for a superblock of the given size class
    superblock_t* sb = NULL; int group;
    for(group = ALLOC_HOARD_FULLNESS_GROUPS - 1; group >= 0; group--) {
        for(sb = heap->bins[size_class][group]; sb; sb = sb->next)
            if(sb->block_used < sb->block_count) break;
        // If a superblock was found break out
        if(sb) break;
    } return sb;
}

//
// Context functions
//

inline size_t context_size() {
    size_t size = sizeof(context_t) - sizeof(char)
                + heap_size() * (util_heapcount() + 1); // Add 1 for global heap
    return util_cachealigned(size);
}

inline heap_t* context_heap(context_t* ctx, int index) {
    return (heap_t*) (ctx->heap_table + heap_size() * index);
}

inline heap_t* context_globalheap(context_t* ctx) {
    return context_heap(ctx, 0);
}

inline heap_t* context_localheap(context_t* ctx, uint32_t threadid) {
    return context_heap(ctx, 1 + (threadid % ctx->heap_count));
}

inline allocation_t* context_allocation_find(context_t* ctx, void* ptr) {
    size_t size = allocation_size();
    return (allocation_t*) ((char*) ctx->blocks_base + ((ptr - ctx->blocks_base) / size) * size);
}

inline superblock_t* context_superblock_find(context_t* ctx, void* ptr) {
    return &context_allocation_find(ctx, ptr)->sb;
}

static void context_init(context_t* ctx) {
    pthread_mutex_init(&ctx->largeblock_lock, NULL);
    ctx->blocks_base = (char*) ctx + context_size();
    ctx->heap_count = util_heapcount();
    ctx->largeblock_next = NULL;
    int i; for(i = 0; i <= ctx->heap_count; i++)
        heap_init(context_heap(ctx, i), i);
}

static void* context_largeblock_malloc(context_t* ctx, size_t sz) {
    largeblock_lock(ctx);
    largeblock_t* lb = largeblock_allocate(ctx, sz);
    void* mem = (lb != NULL) ? largeblock_data(lb) : NULL;
    largeblock_unlock(ctx); return mem;
}

static void* context_superblock_malloc(context_t* ctx, size_t sz) {
    // Get the local heap and lock it
    heap_t* heap = context_localheap(ctx, util_gettid());
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

        // Head of the size class bin on the global heap
        sb = glob->bins[size_class][0];

        // See if there is superblock block available on the global heap
        if(sb) {
            // If it is empty transform it
            if(!sb->block_used) superblock_transform(sb, size_class);

            // Transfer it to the local heap
            superblock_transfer(sb, heap);
        }
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

static void context_largeblock_free(context_t* ctx, largeblock_t* lb, void* ptr) {
    // Add largeblock to free list
    largeblock_lock(ctx);
    largeblock_link(lb, ctx);
    largeblock_unlock(ctx);
}

static void context_superblock_free(context_t* ctx, superblock_t* sb, void* ptr) {
    // Lock the superblock's heap
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

    // Unlock the superblock's heap
    heap_unlock(heap);
}

static void* context_malloc(context_t* ctx, size_t sz) {
    // See if we are allocating a large block
    if(sz > superblock_size()/2)
        return context_largeblock_malloc(ctx, sz);
    // Otherwise perform a standard allocation
    else return context_superblock_malloc(ctx, sz);
}

static void context_free(context_t* ctx, void* ptr) {
    // Fetch the allocation
    allocation_t* alloc = context_allocation_find(ctx, ptr);
    // If it is a normal allocation
    if(alloc->type == ALLOCATION_NORMAL)
        context_superblock_free(ctx, &alloc->sb, ptr);
    // Otherwise perform a large block deallocation
    else context_largeblock_free(ctx, &alloc->lb, ptr);
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
    // Allocate and return the memory
    return context_malloc(get_context(), sz);
}

void mm_free(void *ptr)
{
    // Free the allocation
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
