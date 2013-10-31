// -*- C++ -*-

#ifndef _MM_THREAD_H_
#define _MM_THREAD_H_

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

/* Set thread attributes */

extern void initialize_pthread_attr(int detachstate, int schedpolicy, 
				    int priority, int inheritsched, 
				    int scope, pthread_attr_t *attr);


extern int getNumProcessors (void);

extern void setCPU (int n); 

#endif /* _MM_THREAD_H_ */
