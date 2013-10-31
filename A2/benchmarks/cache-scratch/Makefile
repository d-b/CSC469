INCLUDES = $(TOPDIR)/include
LIBDIR = $(TOPDIR)/util
LIBS = -lmmutil -lpthread
LIBS_DBG = -lmmutil_dbg -lpthread
DEPENDS = cache-scratch.c $(LIBDIR)/libmmutil.a $(INCLUDES)/tsc.h $(INCLUDES)/mm_thread.h $(INCLUDES)/timer.h
DEPENDS_DBG = cache-scratch.c $(LIBDIR)/libmmutil_dbg.a $(INCLUDES)/tsc.h $(INCLUDES)/mm_thread.h $(INCLUDES)/timer.h

CC = gcc
CC_FLAGS = -O3 -Wall -Werror -I$(INCLUDES) -L $(LIBDIR) -D_XOPEN_SOURCE=500
CC_DBG_FLAGS = -g -Wall -Werror -I$(INCLUDES) -L $(LIBDIR) -D_XOPEN_SOURCE=500

all: cache-scratch-kheap cache-scratch-cmu cache-scratch-libc cache-scratch-submit

debug: cache-scratch-kheap-dbg cache-scratch-cmu-dbg cache-scratch-libc-dbg cache-scratch-submit-dbg

submit: cache-scratch-submit

# Allocator based on student submission

cache-scratch-submit: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libsubmit.a
	$(CC) $(CC_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libsubmit.a -lmmutil -lpthread

cache-scratch-submit-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libsubmit_dbg.a
	$(CC) $(CC_DBG_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libsubmit_dbg.a -lmmutil -lpthread

# Allocator based on OS/161 kheap

cache-scratch-kheap: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libkheap.a
	$(CC) $(CC_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libkheap.a -lmmutil -lpthread

cache-scratch-kheap-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libkheap_dbg.a
	$(CC) $(CC_DBG_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libkheap_dbg.a -lmmutil_dbg -lpthread

#Allocator based on CMU CS213 malloc/free

cache-scratch-cmu: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libcmumalloc.a
	$(CC) $(CC_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libcmumalloc.a -lmmutil -lpthread

cache-scratch-cmu-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libcmumalloc_dbg.a
	$(CC) $(CC_DBG_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libcmumalloc_dbg.a -lmmutil_dbg -lpthread

# Allocator using libc malloc/free inside mm_malloc/mm_free wrappers

cache-scratch-libc: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libmmlibc.a
	$(CC) $(CC_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libmmlibc.a -lmmutil -lpthread

cache-scratch-libc-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libmmlibc.a
	$(CC) $(CC_DBG_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libmmlibc.a -lmmutil_dbg -lpthread

# Cleanup
clean:
	rm -f cache-scratch-* *~ 
