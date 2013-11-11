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
    int i; for(i = 0; i < 4; i++) {
        mm_malloc(4096);
    } 
    return 0;
}
