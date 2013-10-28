#ifndef __MEMLIB_H_
#define __MEMLIB_H_

/* $Id$ */

/*
 *  CSC 469 - Assignment 2
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <assert.h>

#define DSEG_MAX 40*1024*1024  /* 40 Mb */

extern char *dseg_lo, *dseg_hi;
extern size_t dseg_size;

extern int mem_init (void);
extern void *mem_sbrk (ptrdiff_t increment);
extern int mem_pagesize (void);
extern ptrdiff_t mem_usage (void);

#endif /* __MEMLIB_H_ */

