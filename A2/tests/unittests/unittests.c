/*
 * CSC469 - Parallel Memory Allocator
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 * Simon Scott <simontupperscott@gmail.com>
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "malloc.h"

int main (int argc, char* argv[]) {
    mm_init();
    int   mem_size = 4;
    void* mem_data = mm_malloc(mem_size);
    printf("mm_malloc(%d): 0x%08x\n", mem_size, (unsigned int) (intptr_t) mem_data);
    return 0;
}
