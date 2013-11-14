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
        printf("%d bytess allocated\n", before);
        mm_free(ptr);
        printf("%d bytess freed\n", before);
        before = before * 2;
    } 
    return 0;
}

int basic_coherency_check(int size){
    int i = 0;
    printf("Coherency check on %d bytes\n", size*4);
    int* array = mm_malloc(size * sizeof(int));
    for (i=0; i < size; i++) array[i] = i;
    for (i=0; i < size; i++) {
        printf(".");
        if (array[i] != i) {
            mm_free(array);
            printf("\nFAILED %d != %d\n", i, array[i]);
            return -1;
        }
    }
    mm_free(array);
    printf("\nPASSED\n");
    return 0;
}

int coherency_check(int depth){

   
	printf("Begin Suberblock coherency check\n");
	/* Create reference array */
	int r = rand() % 2046 ;
	    (void) r;
	int* rand_array = malloc(r * sizeof(int));
	char** ptr_array = malloc(depth * sizeof(char *));

	int i;
	for (i=0; i < r; i++){
			rand_array[i] =  rand() % r; 
	}
	
	/*Allocate in front of test array*/
	for (i=0; i<depth/2 ; i++){
			ptr_array[i] = mm_malloc(rand() % 2046);
	}
	/*Allocate test array*/
	int* test = mm_malloc(r * sizeof(int));
      
	/*Fill test array with reference values*/
    	for (i=0; i < r; i++) test[i] = rand_array[i];	



	printf("Sanity Check\n");
        for (i=0; i < r; i++){
                if(test[i] != rand_array[i]){
                        printf("\nFAILED %d != %d\n", test[i], rand_array[i]);
                        return(-1); 
               }     
                   
        }  
	/*Modify stack*/
	for (i=depth/2; i<depth ; i++){
                        ptr_array[i] = mm_malloc(rand() % 2046);
        }
        for (i=0; i<depth/2 ; i++){
                        mm_free(ptr_array[i]);
        }

	/*TEST*/
	printf("PASS 1\n");
        for (i=0; i < r; i++){
                printf(".");
                if(test[i] != rand_array[i]){
                        printf("\nFAILED %d != %d\n", test[i], rand_array[i]);
			return(-1); 
               }     
                   
        }        
        /*Modify stack*/
	for (i=depth/2; i<depth ; i++){
                        ptr_array[i] = mm_malloc(rand() % 2046);
        }
        for (i=depth/2; i<depth ; i++){
                        mm_free(ptr_array[i]);
        }
        /*TEST*/
        printf("\nPASS 2\n");
        for (i=0; i < r; i++){
                    printf(".");
                if(test[i] != rand_array[i]){
                        printf("\nFAILED %d != %d\n", test[i], rand_array[i]);
                	return(-1);
		}     
                   
        }
	printf("\nPASSED\n");
	return 0;	
}

int main (int argc, char* argv[]) {
    mm_init();
    basic_check();
    basic_coherency_check(100);
    basic_coherency_check(500);
    basic_coherency_check(2000);
    coherency_check(20000);    
return 0;
}
