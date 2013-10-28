INCLUDES = $(TOPDIR)/include
LIBDIR = $(TOPDIR)/util
LIBS = -lmmutil -lpthread
LIBS_DBG = -lmmutil_dbg -lpthread
DEPENDS = threadtest.c $(LIBDIR)/libmmutil.a $(INCLUDES)/tsc.h $(INCLUDES)/mm_thread.h $(INCLUDES)/timer.h
DEPENDS_DBG = threadtest.c $(LIBDIR)/libmmutil_dbg.a $(INCLUDES)/tsc.h $(INCLUDES)/mm_thread.h $(INCLUDES)/timer.h

CC = gcc
CC_FLAGS = -O3 -Wall -Werror -I$(INCLUDES) -L $(LIBDIR) -D_XOPEN_SOURCE=500
CC_DBG_FLAGS = -g -Wall -Werror -I$(INCLUDES) -L $(LIBDIR) -D_XOPEN_SOURCE=500

all: threadtest-kheap threadtest-cmu threadtest-libc threadtest-submit

debug: threadtest-kheap-dbg threadtest-cmu-dbg threadtest-libc-dbg threadtest-submit-dbg

submit: threadtest-submit

# Allocator based on student submission

threadtest-submit: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libsubmit.a
	$(CC) $(CC_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libsubmit.a -lmmutil -lpthread

threadtest-submit-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libsubmit_dbg.a
	$(CC) $(CC_DBG_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libsubmit_dbg.a -lmmutil -lpthread

# Allocator based on OS/161 kheap

threadtest-kheap: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libkheap.a
	$(CC) $(CC_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libkheap.a -lmmutil -lpthread

threadtest-kheap-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libkheap_dbg.a 
	$(CC) $(CC_DBG_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libkheap_dbg.a -lmmutil_dbg -lpthread

#Allocator based on CMU CS213 malloc/free

threadtest-cmu: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libcmumalloc.a
	$(CC) $(CC_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libcmumalloc.a -lmmutil -lpthread

threadtest-cmu-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libcmumalloc_dbg.a 
	$(CC) $(CC_DBG_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libcmumalloc_dbg.a -lmmutil_dbg -lpthread

# Allocator using libc malloc/free inside mm_malloc/mm_free wrappers

threadtest-libc: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libmmlibc.a
	$(CC) $(CC_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libmmlibc.a -lmmutil -lpthread

threadtest-libc-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libmmlibc.a
	$(CC) $(CC_DBG_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libmmlibc.a -lmmutil_dbg -lpthread

# Cleanup
clean:
	rm -f threadtest-* *~ 
