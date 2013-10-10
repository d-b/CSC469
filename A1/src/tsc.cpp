/*
 * CSC469 - Performance Evaluation
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 */

#include "perftest.hpp"

/* Set *hi and *lo to the high and low order bits of the cycle counter.
 * Implementation requires assembly code to use the rdtsc instruction.
 */
inline void access_counter(unsigned int* hi, unsigned int* lo)
{
  asm volatile("rdtsc; movl %%edx, %0; movl %%eax, %1" /* Read cycle counter */
      : "=r" (*hi), "=r" (*lo)                /* and move results to */
      : /* No input */                        /* the two outputs */
      : "%edx", "%eax");
}

/*
 * TSC::now
 *
 * Get the current system cycle count
 */
TSC::cycles TSC::now() {
    unsigned int hi, lo;
    access_counter(&hi, &lo);
    return ((u_int64_t)hi << 32) | lo;
}

/*
 * TSC::start
 *
 * Start the timestamp counter
 */
void TSC::start() {
    unsigned int hi, lo;
    access_counter(&hi, &lo);
    initial = ((u_int64_t)hi << 32) | lo;
}

/*
 * TSC::start
 *
 * Get the current count
 */
TSC::cycles TSC::count() {
    unsigned ncyc_hi, ncyc_lo;
    access_counter(&ncyc_hi, &ncyc_lo);
    return (((cycles)ncyc_hi << 32) | ncyc_lo) - initial;
}
