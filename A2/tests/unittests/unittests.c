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

int coherency_check(void){
	int i = 0;
	printf("Coherency check on 2000 bytes\n");
	char *array = mm_malloc(500 * sizeof(int));
	for (i=0; i < 500; i++){
		array[i] = i;

	}
        for (i=0; i < 500; i++){
		printf(".");
                if (array[i] != i){
			printf("\nFAILED\n");
			return -1;
		}

        }
	printf("\nPASSED\n");
	return 0;
}
int main (int argc, char* argv[]) {
    mm_init();
    basic_check();
    coherency_check();
    int i; for(i = 0; i < 4; i++) {
        mm_malloc(4096);
    } 
    return 0;
}
