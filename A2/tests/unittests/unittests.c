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



int basic_check(void){
	int before = 8;
	void *ptr;
	int i; for(i = 0; before <= 4096; i++) {
        ptr = mm_malloc(before);
	printf("%d bytes allocated\n", before);
	mm_free(ptr);
	printf("%d bytes freed\n", before);
	before = before * 2;
	} 
    return 0;
}
int main (int argc, char* argv[]) {
    mm_init();
    basic_check();
    int i; for(i = 0; i < 4; i++) {
        mm_malloc(4096);
    } 
    return 0;
}
