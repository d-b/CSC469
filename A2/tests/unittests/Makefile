INCLUDES = $(TOPDIR)/include
LIBDIR = $(TOPDIR)/util
LIBS = -lmmutil -lpthread
LIBS_DBG = -lmmutil_dbg -lpthread
DEPENDS = unittests.c $(LIBDIR)/libmmutil.a $(INCLUDES)/tsc.h $(INCLUDES)/mm_thread.h $(INCLUDES)/timer.h
DEPENDS_DBG = unittests.c $(LIBDIR)/libmmutil_dbg.a $(INCLUDES)/tsc.h $(INCLUDES)/mm_thread.h $(INCLUDES)/timer.h

CC = gcc
CC_FLAGS = -O3 -Wall -Werror -I$(INCLUDES) -L $(LIBDIR) -D_XOPEN_SOURCE=500
CC_DBG_FLAGS = -g -Wall -Werror -I$(INCLUDES) -L $(LIBDIR) -D_XOPEN_SOURCE=500

all: unittests-kheap unittests-cmu unittests-libc unittests-submit

debug: unittests-kheap-dbg unittests-cmu-dbg unittests-libc-dbg unittests-submit-dbg

submit: unittests-submit

# Allocator based on student submission

unittests-submit: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libsubmit.a
	$(CC) $(CC_FLAGS) -o $(@) unittests.c $(TOPDIR)/allocators/alloclibs/libsubmit.a -lmmutil -lpthread

unittests-submit-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libsubmit_dbg.a
	$(CC) $(CC_DBG_FLAGS) -o $(@) unittests.c $(TOPDIR)/allocators/alloclibs/libsubmit_dbg.a -lmmutil -lpthread

# Allocator based on OS/161 kheap

unittests-kheap: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libkheap.a
	$(CC) $(CC_FLAGS) -o $(@) unittests.c $(TOPDIR)/allocators/alloclibs/libkheap.a -lmmutil -lpthread

unittests-kheap-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libkheap_dbg.a 
	$(CC) $(CC_DBG_FLAGS) -o $(@) unittests.c $(TOPDIR)/allocators/alloclibs/libkheap_dbg.a -lmmutil_dbg -lpthread

#Allocator based on CMU CS213 malloc/free

unittests-cmu: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libcmumalloc.a
	$(CC) $(CC_FLAGS) -o $(@) unittests.c $(TOPDIR)/allocators/alloclibs/libcmumalloc.a -lmmutil -lpthread

unittests-cmu-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libcmumalloc_dbg.a 
	$(CC) $(CC_DBG_FLAGS) -o $(@) unittests.c $(TOPDIR)/allocators/alloclibs/libcmumalloc_dbg.a -lmmutil_dbg -lpthread

# Allocator using libc malloc/free inside mm_malloc/mm_free wrappers

unittests-libc: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libmmlibc.a
	$(CC) $(CC_FLAGS) -o $(@) unittests.c $(TOPDIR)/allocators/alloclibs/libmmlibc.a -lmmutil -lpthread

unittests-libc-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libmmlibc.a
	$(CC) $(CC_DBG_FLAGS) -o $(@) unittests.c $(TOPDIR)/allocators/alloclibs/libmmlibc.a -lmmutil_dbg -lpthread

# Cleanup
clean:
	rm -f unittests-* *~ 
