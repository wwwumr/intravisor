#include <stdio.h>

void *__expand_heap(size_t *pn)
{
    while(1);
}

static void *ch_start = NULL;
static long ch_ctr = 0;
//static long ch_size = (CONFIG_LINUXU_DEFAULT_HEAPMB*1024*1024/100);
#define CHSIZE  200LL*1024*1024
static long ch_size = CHSIZE;

char dstore[CHSIZE] __attribute__((aligned(4096)));

char* get_memory(unsigned num_bytes) {
//	printf("Get Memory %d\n", num_bytes);
  if(ch_start == NULL) {
	ch_start = dstore;
	ch_ctr = 0;
  }

  if(ch_ctr + num_bytes > ch_size) {
//	uk_pr_crit("OOM, die\n");
	while(1);
  }

  char *ret = (char *) (ch_start + ch_ctr);
  ch_ctr += num_bytes;

  return ret;
}



/*-
 * Copyright (c) 2018-2019 Hongyan Xia
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The original license header is as follows:
 *
 * This is a version (aka dlmalloc) of malloc/free/realloc written by
 * Doug Lea and released to the public domain, as explained at
 * http://creativecommons.org/publicdomain/zero/1.0/ Send questions,
 * comments, complaints, performance data, etc to dl@cs.oswego.edu

 * Version 2.8.6 Wed Aug 29 06:57:58 2012  Doug Lea
 * Note: There may be an updated version of this malloc obtainable at
 *       ftp://gee.cs.oswego.edu/pub/misc/malloc.c. Check before installing!
 */

#include "dlmalloc_nonreuse.h"

void abort(void) {
	__asm__ __volatile__ ("brk 0");
	printf("abort\n");
}


/*------------------------------ internal #includes ---------------------- */

#ifdef CAPREVOKE
#ifndef CHERI_SET_BOUNDS
#error "CHERI_SET_BOUNDS required for CAPREVOKE"
#endif
#endif

#if !NO_MALLOC_STATS
#include <stdio.h>       /* for printing in malloc_stats */
#endif /* NO_MALLOC_STATS */
#ifndef LACKS_ERRNO_H
#include <errno.h>       /* for MALLOC_FAILURE_ACTION */
#endif /* LACKS_ERRNO_H */
#ifdef DEBUG
#if ABORT_ON_ASSERT_FAILURE
#undef assert
#define assert(x) if(!(x)) ABORT
#else /* ABORT_ON_ASSERT_FAILURE */
#include <assert.h>
#endif /* ABORT_ON_ASSERT_FAILURE */
#else  /* DEBUG */
#ifndef assert
#define assert(x)
#endif
#define DEBUG 0
#endif /* DEBUG */
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cherireg.h>
#ifdef CAPREVOKE
#include <sys/caprevoke.h>
#include <sys/stdatomic.h>
#include <cheri/libcaprevoke.h>
#endif /* CAPREVOKE */
#endif /* __CHERI_PURE_CAPABILITY__ */
#if !defined(LACKS_TIME_H)
#include <time.h>        /* for magic initialization */
#endif /* LACKS_TIME_H */
#ifndef LACKS_STDLIB_H
#include <stdlib.h>      /* for abort() */
#endif /* LACKS_STDLIB_H */
#ifndef LACKS_STRING_H
#include <string.h>      /* for memset etc */
#endif  /* LACKS_STRING_H */
#if USE_BUILTIN_FFS
#ifndef LACKS_STRINGS_H
#include <strings.h>     /* for ffs */
#endif /* LACKS_STRINGS_H */
#endif /* USE_BUILTIN_FFS */
#if HAVE_MMAP
#ifndef LACKS_SYS_MMAN_H
// On some versions of linux, __USE_MISC must be set for MAP_ANONYMOUS.
#include<sys/mman.h> // for mmap
#endif /* LACKS_SYS_MMAN_H */
#ifndef LACKS_FCNTL_H
#include <fcntl.h>
#endif /* LACKS_FCNTL_H */
#endif /* HAVE_MMAP */
#ifndef LACKS_UNISTD_H
#include <unistd.h>     /* for sysconf */
#endif /* LACKS_UNISTD_H */

/* Declarations for locking */
#if USE_LOCKS
#if defined (__SVR4) && defined (__sun)  /* solaris */
#include <thread.h>
#elif !defined(LACKS_SCHED_H)
#include <sched.h>
#endif /* solaris or LACKS_SCHED_H */
#if (defined(USE_RECURSIVE_LOCKS) && USE_RECURSIVE_LOCKS != 0) || !USE_SPIN_LOCKS
#include <pthread.h>
#endif /* USE_RECURSIVE_LOCKS ... */
#endif /* USE_LOCKS */

#ifndef LOCK_AT_FORK
#define LOCK_AT_FORK 0
#endif

#ifndef malloc_getpagesize
#  ifdef _SC_PAGESIZE         /* some SVR4 systems omit an underscore */
#    ifndef _SC_PAGE_SIZE
#      define _SC_PAGE_SIZE _SC_PAGESIZE
#    endif
#  endif
#  ifdef _SC_PAGE_SIZE
#    define malloc_getpagesize sysconf(_SC_PAGE_SIZE)
#  else
#    if defined(BSD) || defined(DGUX) || defined(HAVE_GETPAGESIZE)
       extern size_t getpagesize();
#      define malloc_getpagesize getpagesize()
#    else
#        ifndef LACKS_SYS_PARAM_H
#          include <sys/param.h>
#        endif
#        ifdef EXEC_PAGESIZE
#          define malloc_getpagesize EXEC_PAGESIZE
#        else
#          ifdef NBPG
#            ifndef CLSIZE
#              define malloc_getpagesize NBPG
#            else
#              define malloc_getpagesize (NBPG * CLSIZE)
#            endif
#          else
#            ifdef NBPC
#              define malloc_getpagesize NBPC
#            else
#              ifdef PAGESIZE
#                define malloc_getpagesize PAGESIZE
#              else /* just guess */
#                define malloc_getpagesize ((size_t)4096U)
#              endif
#            endif
#          endif
#        endif
#    endif
#  endif
#endif

#ifdef MALLOC_UTRACE
#include<sys/param.h>
#include<sys/uio.h>
#include<sys/ktrace.h>
static int malloc_utrace = 1;
static int malloc_utrace_suspend = 0;

typedef struct {
  void *p;
  size_t s;
  void *r;
} malloc_utrace_t;

#define	UTRACE(a, b, c)\
  if(malloc_utrace && malloc_utrace_suspend == 0) {\
    malloc_utrace_t ut = {a, b, c};\
    utrace(&ut, sizeof(ut));\
  }
#else
#define UTRACE(a, b, c)
#endif

/* -------------------------- Compiler features ----------------------------- */


#ifndef __CHERI_PURE_CAPABILITY__
/* These replacements for alignment builtins are not meant for CHERI purecap
 * due to the potential trouble of recasting to a pointer type. */

#if !__has_builtin(__builtin_align_down)
#define __builtin_align_down(p, a)   (((a) & ((a) - 1)) == 0 ? \
   (__typeof(p))((uintptr_t)(p) & ~((a) - 1)) :                           \
   (__typeof(p))((uintptr_t)(p) - (uintptr_t)(p) % (a)))
#endif   /* !__has_builtin(__builtin_align_down) */

#if !__has_builtin(__builtin_align_up)
#define __builtin_align_up(p, a)   \
   __builtin_align_down((__typeof(p))((uintptr_t)(p) + (a) - 1), a)
#endif   /* !__has_builtin(__builtin_align_up) */

#if !__has_builtin(__builtin_is_aligned)
#define __builtin_is_aligned(p, a)   ((((a) & ((a) - 1)) == 0 ? \
    (uintptr_t)(p) & ((a) - 1) : (uintptr_t)(p) % (a)) == 0)
#endif   /* !__has_builtin(__builtin_is_aligned) */

#endif   /* !__CHERI_PURE_CAPABILITY__ */

/* ------------------- size_t and alignment properties -------------------- */

/* The byte and bit size of a size_t */
#define SIZE_T_SIZE         (sizeof(size_t))
#define SIZE_T_BITSIZE      (sizeof(size_t) << 3)

/* Some constants coerced to size_t */
/* Annoying but necessary to avoid errors on some platforms */
#define SIZE_T_ZERO         ((size_t)0)
#define SIZE_T_ONE          ((size_t)1)
#define SIZE_T_TWO          ((size_t)2)
#define SIZE_T_FOUR         ((size_t)4)
#define SIZE_T_EIGHT        ((size_t)8)
#define SIZE_T_SIXTEEN      ((size_t)16)
#define TWO_SIZE_T_SIZES    (SIZE_T_SIZE<<1)
#define FOUR_SIZE_T_SIZES   (SIZE_T_SIZE<<2)
#define SIX_SIZE_T_SIZES    (FOUR_SIZE_T_SIZES+TWO_SIZE_T_SIZES)
#define HALF_MAX_SIZE_T     (MAX_SIZE_T / 2U)

/* The bit mask value corresponding to MALLOC_ALIGNMENT */
#define CHUNK_ALIGN_MASK    (MALLOC_ALIGNMENT - SIZE_T_ONE)

/* True if address a has acceptable alignment */
#define is_aligned(A)       __builtin_is_aligned((A), MALLOC_ALIGNMENT)

/* the number of bytes to offset an address to align it */
#define align_offset(A)\
  (__builtin_align_up((A), MALLOC_ALIGNMENT) - (A))

/* -------------------------- MMAP preliminaries ------------------------- */

/*
   If HAVE_MMAP is false, we just define calls and
   checks to fail so compiler optimizer can delete code rather than
   using so many "#if"s.
*/

/* MMAP must return MFAIL on failure */
#define MFAIL                ((void*)(MAX_SIZE_T))
#define CMFAIL               ((char*)(MFAIL)) /* defined for convenience */

#if HAVE_MMAP

#define MUNMAP_DEFAULT(a, s)  munmap((a), (s))
#define MMAP_PROT            (PROT_READ|PROT_WRITE)

#if !defined(MAP_ANONYMOUS) && !defined(MAP_ANON)
#error "Anonymous mapping must be provided. #define __USE_MISC for linux?"
#elif !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS        MAP_ANON
#endif // !defined(MAP_ANONYMOUS) && !defined(MAP_ANON)

#define MMAP_FLAGS           (MAP_PRIVATE|MAP_ANONYMOUS|MAP_ALIGNED(PAGE_SHIFT+MALLOC_ALIGN_BITSHIFT))
#define MMAP_DEFAULT(s)       mmap(0, (s), MMAP_PROT, MMAP_FLAGS, -1, 0)

#if defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__)
#define MMAP_SHADOW_FLAGS    (MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT|MAP_FIXED)
#define MMAP_SHADOW(addr, s)       mmap((void*)((size_t)(addr)>>MALLOC_ALIGN_BITSHIFT), (s)>>MALLOC_ALIGN_BITSHIFT, MMAP_PROT, MMAP_SHADOW_FLAGS, -1, 0)
#define MUNMAP_SHADOW(a, s)  munmap((void*)((size_t)(a)>>MALLOC_ALIGN_BITSHIFT), (s)>>MALLOC_ALIGN_BITSHIFT)
#endif   /* defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__) */

#define DIRECT_MMAP_DEFAULT(s) MMAP_DEFAULT(s)
#endif /* HAVE_MMAP */

/**
 * Define CALL_MMAP/CALL_MUNMAP/CALL_DIRECT_MMAP
 */
#if HAVE_MMAP
    #define USE_MMAP_BIT            (SIZE_T_ONE)

    #ifdef MMAP
        #define CALL_MMAP(s)        MMAP(s)
    #else /* MMAP */
        #define CALL_MMAP(s)        MMAP_DEFAULT(s)
    #endif /* MMAP */
    #ifdef MUNMAP
        #define CALL_MUNMAP(a, s)   MUNMAP((a), (s))
    #else /* MUNMAP */
        #define CALL_MUNMAP(a, s)   MUNMAP_DEFAULT((a), (s))
    #endif /* MUNMAP */
    #ifdef DIRECT_MMAP
        #define CALL_DIRECT_MMAP(s) DIRECT_MMAP(s)
    #else /* DIRECT_MMAP */
        #define CALL_DIRECT_MMAP(s) DIRECT_MMAP_DEFAULT(s)
    #endif /* DIRECT_MMAP */
#else  /* HAVE_MMAP */
    #define USE_MMAP_BIT            (SIZE_T_ZERO)

    #define MMAP(s)                 MFAIL
    #define MUNMAP(a, s)            (-1)
    #define DIRECT_MMAP(s)          MFAIL
    #define CALL_DIRECT_MMAP(s)     DIRECT_MMAP(s)
    #define CALL_MMAP(s)            MMAP(s)
    #define CALL_MUNMAP(a, s)       MUNMAP((a), (s))
#endif /* HAVE_MMAP */

#define CALL_MREMAP(addr, osz, nsz, mv)     MFAIL

/* mstate bit set if continguous morecore disabled or failed */
#define USE_NONCONTIGUOUS_BIT (4U)

/* segment bit set in create_mspace_with_base */
#define EXTERN_BIT            (8U)

/* mparam bit indiciaing if we should quarantine or just free */
#define DO_REVOCATION_BIT    (16U)

#if HAVE_MORECORE
#define CALL_MORECORE(S)     MORECORE(S)
#else  /* HAVE_MORECORE */
#define CALL_MORECORE(S)     MFAIL
#endif /* HAVE_MORECORE */


#if USE_LOCKS && HAVE_MORECORE
#define ACQUIRE_MORECORE_LOCK()    ACQUIRE_LOCK(&morecore_mutex);
#define RELEASE_MORECORE_LOCK()    RELEASE_LOCK(&morecore_mutex);
#else /* USE_LOCKS && HAVE_MORECORE */
#define ACQUIRE_MORECORE_LOCK()
#define RELEASE_MORECORE_LOCK()
#endif /* USE_LOCKS && HAVE_MORECORE */


/* --------------------------- Lock preliminaries ------------------------ */

/*
  When locks are defined, there is one global lock, plus
  one per-mspace lock.

  The global lock_ensures that mparams.magic and other unique
  mparams values are initialized only once.

  Per-mspace locks surround calls to malloc, free, etc.
  By default, locks are simple non-reentrant mutexes.

  Because lock-protected regions generally have bounded times, it is
  OK to use the supplied simple spinlocks. Spinlocks are likely to
  improve performance for lightly contended applications, but worsen
  performance under heavy contention.

  If USE_LOCKS is > 1, the definitions of lock routines here are
  bypassed, in which case you will need to define the type MLOCK_T,
  and at least INITIAL_LOCK, DESTROY_LOCK, ACQUIRE_LOCK, RELEASE_LOCK
  and TRY_LOCK.  You must also declare a
    static MLOCK_T malloc_global_mutex = { initialization values };.

*/

#if !USE_LOCKS
#define USE_LOCK_BIT               (0U)
#define INITIAL_LOCK(l)            (0)
#define DESTROY_LOCK(l)            (0)
#define ACQUIRE_MALLOC_GLOBAL_LOCK()
#define RELEASE_MALLOC_GLOBAL_LOCK()

#else
#if USE_LOCKS > 1
/* -----------------------  User-defined locks ------------------------ */
/* Define your own lock implementation here */
/* #define INITIAL_LOCK(lk)  ... */
/* #define DESTROY_LOCK(lk)  ... */
/* #define ACQUIRE_LOCK(lk)  ... */
/* #define RELEASE_LOCK(lk)  ... */
/* #define TRY_LOCK(lk) ... */
/* static MLOCK_T malloc_global_mutex = ... */

#elif USE_SPIN_LOCKS

// First, define CAS_LOCK and CLEAR_LOCK on atomic_flag.
// Note CAS_LOCK defined to return 0 on success.
// Use C11 atomics for portability.

#include<stdatomic.h>

#define CAS_LOCK(sl)     atomic_flag_test_and_set(sl)
#define CLEAR_LOCK(sl)   atomic_flag_clear(sl)

/* How to yield for a spin lock */
#define SPINS_PER_YIELD       63
#if defined (__SVR4) && defined (__sun) /* solaris */
#define SPIN_LOCK_YIELD   thr_yield();
#elif !defined(LACKS_SCHED_H)
#define SPIN_LOCK_YIELD   sched_yield();
#else
#define SPIN_LOCK_YIELD
#endif /* ... yield ... */

#if !defined(USE_RECURSIVE_LOCKS) || USE_RECURSIVE_LOCKS == 0
static size_t lockContended;
/* Plain spin locks use single word (embedded in malloc_states) */
static int
spin_acquire_lock(atomic_flag* sl) {
  lockContended++;
  int spins = 0;
  while(CAS_LOCK(sl)) {
    if((++spins & SPINS_PER_YIELD) == 0) {
      SPIN_LOCK_YIELD;
    }
  }
  return 0;
}

#define MLOCK_T               atomic_flag
#define TRY_LOCK(sl)          !CAS_LOCK(sl)
#define RELEASE_LOCK(sl)      CLEAR_LOCK(sl)
#define ACQUIRE_LOCK(sl)      (CAS_LOCK(sl)? spin_acquire_lock(sl) : 0)
#define INITIAL_LOCK(sl)      (atomic_flag_clear(sl))
#define DESTROY_LOCK(sl)      (0)
static MLOCK_T malloc_global_mutex = ATOMIC_FLAG_INIT;

#else /* USE_RECURSIVE_LOCKS */
/* types for lock owners */

/*
  Note: the following assume that pthread_t is a type that can be
  initialized to (casted) zero. If this is not the case, you will need to
  somehow redefine these or not use spin locks.
*/
#define THREAD_ID_T           pthread_t
#define CURRENT_THREAD        pthread_self()
#define EQ_OWNER(X,Y)         pthread_equal(X, Y)

struct malloc_recursive_lock {
  atomic_flag sl;
  unsigned int c;
  THREAD_ID_T threadid;
};

#define MLOCK_T  struct malloc_recursive_lock
static MLOCK_T malloc_global_mutex = { ATOMIC_FLAG_INIT, 0, (THREAD_ID_T)0};

static FORCEINLINE void recursive_release_lock(MLOCK_T *lk) {
  if (--lk->c == 0) {
    CLEAR_LOCK(&lk->sl);
  }
}

static FORCEINLINE int recursive_acquire_lock(MLOCK_T *lk) {
  THREAD_ID_T mythreadid = CURRENT_THREAD;
  int spins = 0;
  for (;;) {
    if (*((volatile int *)(&lk->sl)) == 0) {
      if (!CAS_LOCK(&lk->sl)) {
        lk->threadid = mythreadid;
        lk->c = 1;
        return 0;
      }
    }
    else if (EQ_OWNER(lk->threadid, mythreadid)) {
      ++lk->c;
      return 0;
    }
    if ((++spins & SPINS_PER_YIELD) == 0) {
      SPIN_LOCK_YIELD;
    }
  }
}

static FORCEINLINE int recursive_try_lock(MLOCK_T *lk) {
  THREAD_ID_T mythreadid = CURRENT_THREAD;
  if (*((volatile int *)(&lk->sl)) == 0) {
    if (!CAS_LOCK(&lk->sl)) {
      lk->threadid = mythreadid;
      lk->c = 1;
      return 1;
    }
  }
  else if (EQ_OWNER(lk->threadid, mythreadid)) {
    ++lk->c;
    return 1;
  }
  return 0;
}

#define RELEASE_LOCK(lk)      recursive_release_lock(lk)
#define TRY_LOCK(lk)          recursive_try_lock(lk)
#define ACQUIRE_LOCK(lk)      recursive_acquire_lock(lk)
#define INITIAL_LOCK(lk)      ((lk)->threadid = (THREAD_ID_T)0, atomic_flag_clear(&(lk)->sl), (lk)->c = 0)
#define DESTROY_LOCK(lk)      (0)
#endif /* USE_RECURSIVE_LOCKS */

#else /* pthreads-based locks */
#define MLOCK_T               pthread_mutex_t
#define ACQUIRE_LOCK(lk)      pthread_mutex_lock(lk)
#define RELEASE_LOCK(lk)      pthread_mutex_unlock(lk)
#define TRY_LOCK(lk)          (!pthread_mutex_trylock(lk))
#define INITIAL_LOCK(lk)      pthread_init_lock(lk)
#define DESTROY_LOCK(lk)      pthread_mutex_destroy(lk)

static MLOCK_T malloc_global_mutex = PTHREAD_MUTEX_INITIALIZER;

static int pthread_init_lock (MLOCK_T *lk) {
  pthread_mutexattr_t attr;
  if (pthread_mutexattr_init(&attr)) return 1;
#if defined(USE_RECURSIVE_LOCKS) && USE_RECURSIVE_LOCKS != 0
  if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)) return 1;
#endif
  if (pthread_mutex_init(lk, &attr)) return 1;
  if (pthread_mutexattr_destroy(&attr)) return 1;
  return 0;
}

#endif /* ... lock types ... */

/* Common code for all lock types */
#define USE_LOCK_BIT               (2U)

#ifndef ACQUIRE_MALLOC_GLOBAL_LOCK
#define ACQUIRE_MALLOC_GLOBAL_LOCK()  ACQUIRE_LOCK(&malloc_global_mutex);
#endif

#ifndef RELEASE_MALLOC_GLOBAL_LOCK
#define RELEASE_MALLOC_GLOBAL_LOCK()  RELEASE_LOCK(&malloc_global_mutex);
#endif

#endif /* USE_LOCKS */

/* -----------------------  Chunk representations ------------------------ */

/*
  (The following includes lightly edited explanations by Colin Plumb.)

  The malloc_chunk declaration below is misleading (but accurate and
  necessary).  It declares a "view" into memory allowing access to
  necessary fields at known offsets from a given base.

  Chunks of memory are maintained using a `boundary tag' method as
  originally described by Knuth.  (See the paper by Paul Wilson
  ftp://ftp.cs.utexas.edu/pub/garbage/allocsrv.ps for a survey of such
  techniques.)  Sizes of free chunks are stored both in the front of
  each chunk and at the end.  This makes consolidating fragmented
  chunks into bigger chunks fast.  The head fields also hold bits
  representing whether chunks are free or in use.

  Here are some pictures to make it clearer.  They are "exploded" to
  show that the state of a chunk can be thought of as extending from
  the high 31 bits of the head field of its header through the
  prev_foot and PINUSE_BIT bit of the following chunk header.

  A chunk that's in use looks like:

   chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           | Size of previous chunk (if P = 0)                             |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |P|
         | Size of this chunk                                         1| +-+
   mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                                                               |
         +-                                                             -+
         |                                                               |
         +-                                                             -+
         |                                                               :
         +-      size - sizeof(size_t) available payload bytes          -+
         :                                                               |
 chunk-> +-                                                             -+
         |                                                               |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |1|
       | Size of next chunk (may or may not be in use)               | +-+
 mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    And if it's free, it looks like this:

   chunk-> +-                                                             -+
           | User payload (must be in use, or we would have merged!)       |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |P|
         | Size of this chunk                                         0| +-+
   mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Next pointer                                                  |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Prev pointer                                                  |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                                                               :
         +-      size - sizeof(struct chunk) unused bytes               -+
         :                                                               |
 chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Size of this chunk                                            |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |0|
       | Size of next chunk (must be in use, or we would have merged)| +-+
 mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                                                               :
       +- User payload                                                -+
       :                                                               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                                                     |0|
                                                                     +-+
  Note that since we always merge adjacent free chunks, the chunks
  adjacent to a free chunk must be in use.

  Given a pointer to a chunk (which can be derived trivially from the
  payload pointer) we can, in O(1) time, find out whether the adjacent
  chunks are free, and if so, unlink them from the lists that they
  are on and merge them with the current chunk.

  Chunks always begin on even word boundaries, so the mem portion
  (which is returned to the user) is also on an even word boundary, and
  thus at least double-word aligned.

  The P (PINUSE_BIT) bit, stored in the unused low-order bit of the
  chunk size (which is always a multiple of two words), is an in-use
  bit for the *previous* chunk.  If that bit is *clear*, then the
  word before the current chunk size contains the previous chunk
  size, and can be used to find the front of the previous chunk.
  The very first chunk allocated always has this bit set, preventing
  access to non-existent (or non-owned) memory. If pinuse is set for
  any given chunk, then you CANNOT determine the size of the
  previous chunk, and might even get a memory addressing fault when
  trying to do so.

  The C (CINUSE_BIT) bit, stored in the unused second-lowest bit of
  the chunk size redundantly records whether the current chunk is
  inuse (unless the chunk is mmapped). This redundancy enables usage
  checks within free and realloc, and reduces indirection when freeing
  and consolidating chunks.

  Each freshly allocated chunk must have both cinuse and pinuse set.
  That is, each allocated chunk borders either a previously allocated
  and still in-use chunk, or the base of its memory arena. This is
  ensured by making all allocations from the `lowest' part of any
  found chunk.  Further, no free chunk physically borders another one,
  so each free chunk is known to be preceded and followed by either
  inuse chunks or the ends of memory.

  Note that the `foot' of the current chunk is actually represented
  as the prev_foot of the NEXT chunk. This makes it easier to
  deal with alignments etc but can be very confusing when trying
  to extend or adapt this code.

  The exceptions to all this are

     1. The special chunk `top' is the top-most available chunk (i.e.,
        the one bordering the end of available memory). It is treated
        specially.  Top is never included in any bin, is used only if
        no other chunk is available, and is released back to the
        system if it is very large (see M_TRIM_THRESHOLD).  In effect,
        the top chunk is treated as larger (and thus less well
        fitting) than any other available chunk.  The top chunk
        doesn't update its trailing size field since there is no next
        contiguous chunk that would have to index off it. However,
        space is still allocated for it (TOP_FOOT_SIZE) to enable
        separation or merging when space is extended.

     3. Chunks allocated via mmap, have both cinuse and pinuse bits
        cleared in their head fields.  Because they are allocated
        one-by-one, each must carry its own prev_foot field, which is
        also used to hold the offset this chunk has within its mmapped
        region, which is needed to preserve alignment. Each mmapped
        chunk is trailed by the first two fields of a fake next-chunk
        for sake of usage checks.

*/

struct malloc_chunk {
  size_t               prev_foot;  /* Size of previous chunk (if free).  */
  size_t               head;       /* Size and inuse bits. */
#ifdef __CHERI_PURE_CAPABILITY__
  /*
   * It is a documented requirement that struct malloc_chunk be a power
   * of two in size.
   */
  void* pad;
#endif
  struct malloc_chunk* fd;         /* double links -- used only if free. */
  struct malloc_chunk* bk;
};

typedef struct malloc_chunk  mchunk;
typedef struct malloc_chunk* mchunkptr;
typedef struct malloc_chunk* sbinptr;  /* The type of bins of chunks */
typedef unsigned int bindex_t;         /* Described below */
typedef unsigned int binmap_t;         /* Described below */
typedef unsigned int flag_t;           /* The type of various bit flag sets */

/* ------------------- Chunks sizes and alignments ----------------------- */

#define MCHUNK_SIZE         (sizeof(mchunk))

#ifdef SAFE_FREEBUF
#define	CHUNK_HEADER_OFFSET	sizeof(struct malloc_chunk)
#else
#define	CHUNK_HEADER_OFFSET	__offsetof(struct malloc_chunk, fd)
#endif

#define CHUNK_OVERHEAD      (CHUNK_HEADER_OFFSET)

/* MMapped chunks need a second word of overhead ... */
#define MMAP_CHUNK_OVERHEAD (TWO_SIZE_T_SIZES)
/* ... and additional padding for fake next-chunk at foot */
#define MMAP_FOOT_PAD       (FOUR_SIZE_T_SIZES)

/* The smallest size we can malloc is an aligned minimal chunk */
#ifdef SAFE_FREEBUF
#define MIN_CHUNK_SIZE \
  (((2 * MCHUNK_SIZE) + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)
#else
#define MIN_CHUNK_SIZE\
  ((MCHUNK_SIZE + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)
#endif

/* conversion from malloc headers to user pointers, and back */
#define chunk2mem(p)        ((void*)((char*)(p)       + CHUNK_HEADER_OFFSET))
#define mem2chunk(mem)      ((mchunkptr)((char*)(mem) - CHUNK_HEADER_OFFSET))
/* chunk associated with aligned address A */
#define align_as_chunk(A)   (mchunkptr)((A) + align_offset(chunk2mem(A)))

/* Bounds on request (not chunk) sizes. */
#define MAX_REQUEST         ((-MIN_CHUNK_SIZE) << 2)
#define MIN_REQUEST         (MIN_CHUNK_SIZE - CHUNK_OVERHEAD - SIZE_T_ONE)

/* pad request bytes into a usable size */
#define pad_request(req) \
   (((req) + CHUNK_OVERHEAD + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

/* pad request, checking for minimum (but not maximum) */
#define request2size(req) \
  (((req) < MIN_REQUEST)? MIN_CHUNK_SIZE : pad_request(req))


/* ------------------ Operations on head and foot fields ----------------- */

/*
  The head field of a chunk is or'ed with PINUSE_BIT when previous
  adjacent chunk in use, and or'ed with CINUSE_BIT if this chunk is in
  use, unless mmapped, in which case both bits are cleared.

  CDIRTY_BIT indicates whether this freed chunk is dirty (not swept) or not.
  PDIRTY_BIT indicates whether the previous chunk is in dirty.
  CUNMAPPED_BIT indicates that the part aligned portion of the (dirty)
                chunk are unmapped.
*/

#define PINUSE_BIT          (SIZE_T_ONE)
#define CINUSE_BIT          (SIZE_T_TWO)
#define PDIRTY_BIT          (SIZE_T_FOUR)
#define CDIRTY_BIT          (SIZE_T_EIGHT)
#define CUNMAPPED_BIT       (SIZE_T_SIXTEEN)
#define INUSE_BITS          (PINUSE_BIT|CINUSE_BIT)
#define DIRTY_BITS          (PDIRTY_BIT|CDIRTY_BIT)
#define FLAG_BITS           (PINUSE_BIT|CINUSE_BIT|CDIRTY_BIT|PDIRTY_BIT|CUNMAPPED_BIT)

/* Head value for fenceposts */
#define FENCEPOST_HEAD      (INUSE_BITS)

/* extraction of fields from head words */
#define cinuse(p)           ((p)->head & CINUSE_BIT)
#define pinuse(p)           ((p)->head & PINUSE_BIT)
#define cdirty(p)           ((p)->head & CDIRTY_BIT)
#define pdirty(p)           ((p)->head & PDIRTY_BIT)
#define cunmapped(p)        ((p)->head & CUNMAPPED_BIT)
#define dirtybits(p)        ((p)->head & (PDIRTY_BIT|CDIRTY_BIT))
#define inusebits(p)        ((p)->head & (PINUSE_BIT|CINUSE_BIT|CUNMAPPED_BIT))
#define is_inuse(p)         ((((p)->head & INUSE_BITS) != PINUSE_BIT))
#define is_mmapped(p)       (((p)->head & INUSE_BITS) == 0)

#define chunksize(p)        ((p)->head & ~(FLAG_BITS))

#define clear_pinuse(p)     ((p)->head &= ~PINUSE_BIT)
#define clear_cinuse(p)     ((p)->head &= ~CINUSE_BIT)
#define set_cdirty(p)       ((p)->head |= CDIRTY_BIT)
#define clear_cdirty(p)     ((p)->head &= ~CDIRTY_BIT)
#define set_pdirty(p)       ((p)->head |= PDIRTY_BIT)
#define clear_pdirty(p)     ((p)->head &= ~PDIRTY_BIT)
#define set_cunmapped(p)       ((p)->head |= CUNMAPPED_BIT)
#define clear_cunmapped(p)     ((p)->head &= ~CUNMAPPED_BIT)

/* Treat space at ptr +/- offset as a chunk */
#define chunk_plus_offset(p, s)  ((mchunkptr)(((char*)(p)) + (s)))
#define chunk_minus_offset(p, s) ((mchunkptr)(((char*)(p)) - (s)))

/* Ptr to next or previous physical malloc_chunk. */
#define next_chunk(p) ((mchunkptr)( ((char*)(p)) + chunksize(p) ))
#define prev_chunk(p) ((mchunkptr)( ((char*)(p)) - ((p)->prev_foot) ))

/* extract next chunk's pinuse bit */
#define next_pinuse(p)  ((next_chunk(p)->head) & PINUSE_BIT)

/* Get/set size at footer */
#define get_foot(p, s)  (((mchunkptr)((char*)(p) + (s)))->prev_foot)
#define set_foot(p, s)  (((mchunkptr)((char*)(p) + (s)))->prev_foot = (s))

#define set_size_and_clear_pdirty_of_dirty_chunk(p, s)\
  ((p)->head = (inusebits(p)|s|CDIRTY_BIT), set_foot(p, s))

/* Set size, pinuse bit, and foot */
#define set_size_and_pinuse_of_free_chunk(p, s)\
  ((p)->head = (pdirty(p)|s|PINUSE_BIT), set_foot(p, s))

/* Set size, pinuse bit, foot, and clear next pinuse */
#define set_free_with_pinuse(p, s, n)\
  (clear_pinuse(n), clear_pdirty(n), set_size_and_pinuse_of_free_chunk(p, s))

/* Get the internal overhead associated with chunk p */
#define overhead_for(p)\
 (is_mmapped(p)? MMAP_CHUNK_OVERHEAD : CHUNK_OVERHEAD)

/* Return true if malloced space is not necessarily cleared */
#if MMAP_CLEARS
#define calloc_must_clear(p) (!is_mmapped(p))
#else /* MMAP_CLEARS */
#define calloc_must_clear(p) (1)
#endif /* MMAP_CLEARS */

#define clear_meta(P, S)\
  if (is_small(S)) memset(P, 0, sizeof(mchunk));\
  else memset((P), 0, sizeof(tchunk));

/* ---------------------- Overlaid data structures ----------------------- */

/*
  When chunks are not in use, they are treated as nodes of either
  lists or trees.

  "Small"  chunks are stored in circular doubly-linked lists, and look
  like this:

    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk                            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Forward pointer to next chunk in list             |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Back pointer to previous chunk in list            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Unused space (may be 0 bytes long)                .
            .                                                               .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Larger chunks are kept in a form of bitwise digital trees (aka
  tries) keyed on chunksizes.  Because malloc_tree_chunks are only for
  free chunks greater than 256 bytes, their size doesn't impose any
  constraints on user chunk sizes.  Each node looks like:

    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk                            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Forward pointer to next chunk of same size        |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Back pointer to previous chunk of same size       |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to left child (child[0])                  |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to right child (child[1])                 |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to parent                                 |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             bin index of this chunk                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Unused space                                      .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Each tree holding treenodes is a tree of unique chunk sizes.  Chunks
  of the same size are arranged in a circularly-linked list, with only
  the oldest chunk (the next to be used, in our FIFO ordering)
  actually in the tree.  (Tree members are distinguished by a non-null
  parent pointer.)  If a chunk with the same size an an existing node
  is inserted, it is linked off the existing node using pointers that
  work in the same way as fd/bk pointers of small chunks.

  Each tree contains a power of 2 sized range of chunk sizes (the
  smallest is 0x100 <= x < 0x180), which is is divided in half at each
  tree level, with the chunks in the smaller half of the range (0x100
  <= x < 0x140 for the top nose) in the left subtree and the larger
  half (0x140 <= x < 0x180) in the right subtree.  This is, of course,
  done by inspecting individual bits.

  Using these rules, each node's left subtree contains all smaller
  sizes than its right subtree.  However, the node at the root of each
  subtree has no particular ordering relationship to either.  (The
  dividing line between the subtree sizes is based on trie relation.)
  If we remove the last chunk of a given size from the interior of the
  tree, we need to replace it with a leaf node.  The tree ordering
  rules permit a node to be replaced by any leaf below it.

  The smallest chunk in a tree (a common operation in a best-fit
  allocator) can be found by walking a path to the leftmost leaf in
  the tree.  Unlike a usual binary tree, where we follow left child
  pointers until we reach a null, here we follow the right child
  pointer any time the left one is null, until we reach a leaf with
  both child pointers null. The smallest chunk in the tree will be
  somewhere along that path.

  The worst case number of steps to add, find, or remove a node is
  bounded by the number of bits differentiating chunks within
  bins. Under current bin calculations, this ranges from 6 up to 21
  (for 32 bit sizes) or up to 53 (for 64 bit sizes). The typical case
  is of course much better.
*/

struct malloc_tree_chunk {
  /* The first four(five for CHERI) fields must be compatible with malloc_chunk */
  size_t                    prev_foot;
  size_t                    head;
#ifdef __CHERI_PURE_CAPABILITY__
  /*
   * It is a documented requirement that struct malloc_chunk be a power
   * of two in size.
   */
  void* pad;
#endif
  struct malloc_tree_chunk* fd;
  struct malloc_tree_chunk* bk;

  struct malloc_tree_chunk* child[2];
  struct malloc_tree_chunk* parent;
  bindex_t                  index;
};

typedef struct malloc_tree_chunk  tchunk;
typedef struct malloc_tree_chunk* tchunkptr;
typedef struct malloc_tree_chunk* tbinptr; /* The type of bins of trees */

/* A little helper macro for trees */
#define leftmost_child(t) ((t)->child[0] != 0? (t)->child[0] : (t)->child[1])

/* ----------------------------- Segments -------------------------------- */

/*
  Each malloc space may include non-contiguous segments, held in a
  list headed by an embedded malloc_segment record representing the
  top-most space. Segments also include flags holding properties of
  the space. Large chunks that are directly allocated by mmap are not
  included in this list. They are instead independently created and
  destroyed without otherwise keeping track of them.

  Segment management mainly comes into play for spaces allocated by
  MMAP.  Any call to MMAP might or might not return memory that is
  adjacent to an existing segment.
  When allocating using MMAP, we don't use any of the
  hinting mechanisms (inconsistently) supported in various
  implementations of unix mmap, or distinguish reserving from
  committing memory. Instead, we just ask for space, and exploit
  contiguity when we get it.  It is probably possible to do
  better than this on some systems, but no general scheme seems
  to be significantly better.

  Management entails a simpler variant of the consolidation scheme
  used for chunks to reduce fragmentation -- new adjacent memory is
  normally prepended or appended to an existing segment. However,
  there are limitations compared to chunk consolidation that mostly
  reflect the fact that segment processing is relatively infrequent
  (occurring only when getting memory from system) and that we
  don't expect to have huge numbers of segments:

  * Segments are not indexed, so traversal requires linear scans.  (It
    would be possible to index these, but is not worth the extra
    overhead and complexity for most programs on most platforms.)
  * New segments are only appended to old ones when holding top-most
    memory; if they cannot be prepended to others, they are held in
    different segments.

  Except for the top-most segment of an mstate, each segment record
  is kept at the tail of its segment. Segments are added by pushing
  segment records onto the list headed by &mstate.seg for the
  containing mstate.

  Segment flags control allocation/merge/deallocation policies:
  * If EXTERN_BIT set, then we did not allocate this segment,
    and so should not try to deallocate or merge with others.
    (This currently holds only for the initial segment passed
    into create_mspace_with_base.)
  * If USE_MMAP_BIT set, the segment may be merged with
    other surrounding mmapped segments and trimmed/de-allocated
    using munmap.
*/

struct malloc_segment {
  char*        base;             /* base address */
  size_t       size;             /* allocated size */
  struct malloc_segment* next;   /* ptr to next segment */
  flag_t       sflags;           /* mmap and extern flag */
#ifdef CAPREVOKE
  void*        shadow;
#endif
};

#define is_mmapped_segment(S)  ((S)->sflags & USE_MMAP_BIT)
#define is_extern_segment(S)   ((S)->sflags & EXTERN_BIT)

typedef struct malloc_segment  msegment;
typedef struct malloc_segment* msegmentptr;

/* ---------------------------- malloc_state ----------------------------- */

/*
   A malloc_state holds all of the bookkeeping for a space.
   The main fields are:

  Top
    The topmost chunk of the currently active segment. Its size is
    cached in topsize.  The actual size of topmost space is
    topsize+TOP_FOOT_SIZE, which includes space reserved for adding
    fenceposts and segment records if necessary when getting more
    space from the system.  The size at which to autotrim top is
    cached from mparams in trim_check, except that it is disabled if
    an autotrim fails.

  Designated victim (dv)
    This is the preferred chunk for servicing small requests that
    don't have exact fits.  It is normally the chunk split off most
    recently to service another small request.  Its size is cached in
    dvsize. The link fields of this chunk are not maintained since it
    is not kept in a bin.

  SmallBins
    An array of bin headers for free chunks.  These bins hold chunks
    with sizes less than MIN_LARGE_SIZE bytes. Each bin contains
    chunks of all the same size, spaced 8 bytes apart.  To simplify
    use in double-linked lists, each bin header acts as a malloc_chunk
    pointing to the real first node, if it exists (else pointing to
    itself).  This avoids special-casing for headers.  But to avoid
    waste, we allocate only the fd/bk pointers of bins, and then use
    repositioning tricks to treat these as the fields of a chunk.

  TreeBins
    Treebins are pointers to the roots of trees holding a range of
    sizes. There are 2 equally spaced treebins for each power of two
    from TREE_SHIFT to TREE_SHIFT+16. The last bin holds anything
    larger.

  Bin maps
    There is one bit map for small bins ("smallmap") and one for
    treebins ("treemap).  Each bin sets its bit when non-empty, and
    clears the bit when empty.  Bit operations are then used to avoid
    bin-by-bin searching -- nearly all "search" is done without ever
    looking at bins that won't be selected.  The bit maps
    conservatively use 32 bits per map word, even if on 64bit system.
    For a good description of some of the bit-based techniques used
    here, see Henry S. Warren Jr's book "Hacker's Delight" (and
    supplement at http://hackersdelight.org/). Many of these are
    intended to reduce the branchiness of paths through malloc etc, as
    well as to reduce the number of memory locations read or written.

  Segments
    A list of segments headed by an embedded malloc_segment record
    representing the initial space.

  Address check support
    The least_addr field is the least address ever obtained from
    MMAP. Attempted frees and reallocs of any address less
    than this are trapped (unless INSECURE is defined).

  Magic tag
    A cross-check field that should always hold same value as mparams.magic.

  Max allowed footprint
    The maximum allowed bytes to allocate from system (zero means no limit)

  Flags
    Bits recording whether to use MMAP, locks

  Statistics
    Each space keeps track of current and maximum system memory
    obtained via MMAP.

  Trim support
    Fields holding the amount of unused topmost memory that should trigger
    trimming, and a counter to force periodic scanning to release unused
    non-topmost segments.

  Locking
    If USE_LOCKS is defined, the "mutex" lock is acquired and released
    around every public call using this mspace.

  Extension support
    A void* pointer and a size_t field that can be used to help implement
    extensions to this malloc.
*/

/* Bin types, widths and sizes */
#define NSMALLBINS        (32U)
#define NTREEBINS         (32U)
#define SMALLBIN_SHIFT    (3U)
#define SMALLBIN_WIDTH    (SIZE_T_ONE << SMALLBIN_SHIFT)
#define TREEBIN_SHIFT     (8U)
#define MIN_LARGE_SIZE    (SIZE_T_ONE << TREEBIN_SHIFT)
#define MAX_SMALL_SIZE    (MIN_LARGE_SIZE - SIZE_T_ONE)
#define MAX_SMALL_REQUEST (MAX_SMALL_SIZE - CHUNK_ALIGN_MASK - CHUNK_OVERHEAD)

struct malloc_state {
  binmap_t   smallmap;
  binmap_t   treemap;
  size_t     freebufbytes;
  size_t     freebufbytes_unmapped; /* freebufbytes - freebufbytes_mapped */
  size_t     freebufsize;
  size_t     dvsize;
  size_t     topsize;
  char*      least_addr;
  mchunkptr  dv;
  mchunkptr  top;
  size_t     trim_check;
  size_t     release_checks;
  size_t     magic;
  mchunkptr  smallbins[(NSMALLBINS+1)*2];
  tbinptr    treebins[NTREEBINS];
  mchunk     freebufbin;
  size_t     footprint;
  size_t     max_footprint;
  size_t     footprint_limit; /* zero means no limit */
  size_t     allocated;
  size_t     max_allocated;
#if SWEEP_STATS
  size_t     sweepTimes;
  size_t     sweptBytes;
  size_t     freeTimes;
  size_t     freeBytes;
  size_t     bitsPainted;
  size_t     bitsCleared;
#endif // SWEEP_STATS
  flag_t     mflags;
#if USE_LOCKS
  MLOCK_T    mutex;     /* locate lock among fields that rarely change */
#endif /* USE_LOCKS */
  msegment   seg;
  void*      extp;      /* Unused but available for extensions */
  size_t     exts;
};

typedef struct malloc_state*    mstate;

/* ------------- Global malloc_state and malloc_params ------------------- */

/*
  malloc_params holds global properties, including those that can be
  dynamically set using mallopt. There is a single instance, mparams,
  initialized in init_mparams. Note that the non-zeroness of "magic"
  also serves as an initialization flag.
*/

struct malloc_params {
  size_t magic;
  size_t page_size;;
  size_t granularity;
  size_t mmap_threshold;
  size_t trim_threshold;
  size_t min_freebufbytes;
  size_t max_freebufbytes;
  double max_freebuf_percent;
#if SUPPORT_UNMAP
  size_t unmap_threshold;
#endif
  flag_t default_mflags;
};

static struct malloc_params mparams;

/* Ensure mparams initialized */
#define ensure_initialization() (void)(mparams.magic != 0 || init_mparams())

/* The global malloc_state used for all non-"mspace" calls */
static struct malloc_state _gm_;
#define gm                 (&_gm_)
#define is_global(M)       ((M) == &_gm_)

#define is_initialized(M)  ((M)->top != 0)

#ifdef CAPREVOKE
static volatile const struct caprevoke_info *cri;
#endif

/* -------------------------- system alloc setup ------------------------- */

/* Operations on mflags */

#define use_lock(M)           ((M)->mflags &   USE_LOCK_BIT)
#define enable_lock(M)        ((M)->mflags |=  USE_LOCK_BIT)
#if USE_LOCKS
#define disable_lock(M)       ((M)->mflags &= ~USE_LOCK_BIT)
#else
#define disable_lock(M)
#endif

#define use_mmap(M)           ((M)->mflags &   USE_MMAP_BIT)
#define enable_mmap(M)        ((M)->mflags |=  USE_MMAP_BIT)
#if HAVE_MMAP
#define disable_mmap(M)       ((M)->mflags &= ~USE_MMAP_BIT)
#else
#define disable_mmap(M)
#endif

#define use_noncontiguous(M)  ((M)->mflags &   USE_NONCONTIGUOUS_BIT)
#define disable_contiguous(M) ((M)->mflags |=  USE_NONCONTIGUOUS_BIT)

#define do_revocation(M)      ((M)->mflags &   DO_REVOCATION_BIT)
#define enable_revocation(M)  ((M)->mflags |=  DO_REVOCATION_BIT)
#ifdef ALLOW_DISABLING_REVOCATION
#define disable_revocation(M) ((M)->mflags &= ~DO_REVOCATION_BIT)
#else
#define disable_revocation(M)
#endif

#define set_lock(M,L)\
 ((M)->mflags = (L)?\
  ((M)->mflags | USE_LOCK_BIT) :\
  ((M)->mflags & ~USE_LOCK_BIT))

/* page-align a size */
#define page_align(S)\
  __builtin_align_up((S), mparams.page_size)

/* granularity-align a size */
#ifndef __CHERI_PURE_CAPABILITY__
#define granularity_align(S)\
  __builtin_align_up((S), mparams.granularity)
#else
#define granularity_align(S) \
  __builtin_cheri_round_representable_length(__builtin_align_up((S), \
					     mparams.granularity))
#endif

#define mmap_align(S) page_align(S)

/* For sys_alloc, enough padding to ensure can malloc request on success */
#define SYS_ALLOC_PADDING (TOP_FOOT_SIZE + MALLOC_ALIGNMENT)

#define is_page_aligned(S)\
  __builtin_is_aligned((S), mparams.page_size)
#define is_granularity_aligned(S)\
  __builtin_is_aligned((S), mparams.granularity)

/*  True if segment S holds address A */
#define segment_holds(S, A)\
  ((char*)(A) >= S->base && (char*)(A) < S->base + S->size)

/* Return segment holding given address */
static msegmentptr segment_holding(mstate m, char* addr) {
  msegmentptr sp = &m->seg;
  for (;;) {
    if (addr >= sp->base && addr < sp->base + sp->size)
      return sp;
    if ((sp = sp->next) == 0)
      return 0;
  }
}

/* Return true if segment contains a segment link */
static int has_segment_link(mstate m, msegmentptr ss) {
  msegmentptr sp = &m->seg;
  for (;;) {
    if ((char*)sp >= ss->base && (char*)sp < ss->base + ss->size)
      return 1;
    if ((sp = sp->next) == 0)
      return 0;
  }
}

#define should_trim(M,s)  ((s) > (M)->trim_check)

/*
  TOP_FOOT_SIZE is padding at the end of a segment, including space
  that may be needed to place segment records and fenceposts when new
  noncontiguous segments are added.
*/
#define TOP_FOOT_SIZE\
  (align_offset(CHUNK_HEADER_OFFSET)+pad_request(sizeof(struct malloc_segment))+MIN_CHUNK_SIZE)


/* -------------------------------  Hooks -------------------------------- */

/*
  PREACTION should be defined to return 0 on success, and nonzero on
  failure. If you are not using locking, you can redefine these to do
  anything you like.
*/

#if USE_LOCKS
#define PREACTION(M)  ((use_lock(M))? ACQUIRE_LOCK(&(M)->mutex) : 0)
#define POSTACTION(M) { if (use_lock(M)) RELEASE_LOCK(&(M)->mutex); }
#else /* USE_LOCKS */

#ifndef PREACTION
#define PREACTION(M) (0)
#endif  /* PREACTION */

#ifndef POSTACTION
#define POSTACTION(M)
#endif  /* POSTACTION */

#endif /* USE_LOCKS */

/*
  CORRUPTION_ERROR_ACTION is triggered upon detected bad addresses.
  USAGE_ERROR_ACTION is triggered on detected bad frees and
  reallocs. The argument p is an address that might have triggered the
  fault. It is ignored by the two predefined actions, but might be
  useful in custom actions that try to help diagnose errors.
*/

// Hopefully snprintf() won't call malloc().
#define malloc_printf(...) do { \
    printf(__VA_ARGS__); \
} while(0);

__attribute((optnone))
static inline void usage_error(mstate m, void *mem) {
    malloc_printf("USAGE ERROR: mstate=%p, mem=%#p\n", m, mem);
    abort();
}

__attribute((optnone))
static inline void corruption_error(mstate m) {
    malloc_printf("CORRUPTION ERROR: mstate=%#p\n", m);
    abort();
}

#ifndef CORRUPTION_ERROR_ACTION
#define CORRUPTION_ERROR_ACTION(m) corruption_error(m)
#endif /* CORRUPTION_ERROR_ACTION */

#ifndef USAGE_ERROR_ACTION
#define USAGE_ERROR_ACTION(m,p) usage_error(m, p)
#endif /* USAGE_ERROR_ACTION */

/* --------------------------- CHERI support ----------------------------- */

/*
 * Bound a memory allocation and remove unneeded permissions.
 */
#ifndef __CHERI_PURE_CAPABILITY__
#define	bound_ptr(mem, bytes)	(mem)
#else
static inline void *bound_ptr(void *mem, size_t bytes)
{
	void* ptr;

#ifdef CHERI_SET_BOUNDS
	ptr = __builtin_cheri_bounds_set(mem, bytes);
#else
	ptr = mem;
#endif
//	ptr = __builtin_cheri_perms_and(ptr,
//	    CHERI_PERMS_USERSPACE_DATA & ~CHERI_PERM_CHERIABI_VMMAP);
	mem2chunk(mem)->pad = mem;
	return ptr;
}
#endif

/*
 * Given a memory allocation, return a pointer to it bounded to the
 * segment it was allocated from.
 */
#ifndef __CHERI_PURE_CAPABILITY__
#define	unbound_ptr(m, spp, mem)	(mem)
#else
static inline void *unbound_ptr(mstate m, msegmentptr *spp, void *mem)
{
	void* ptr;
#ifdef CHERI_SET_BOUNDS
	msegmentptr sp;

	if (__builtin_cheri_tag_get(mem) != 1 ||
	    __builtin_cheri_offset_get(mem) != 0)
		USAGE_ERROR_ACTION(m, mem);
	sp = segment_holding(m, mem);
	if (sp == NULL)
		USAGE_ERROR_ACTION(m, mem);
	if (spp != NULL)
		*spp = sp;
	ptr = sp->base + ((char *)mem - (char *)sp->base);
#else
	if (__builtin_cheri_tag_get(mem) != 1)
		USAGE_ERROR_ACTION(m, mem);
	ptr = mem;
#endif

#if 0
	printf(" %x %x %x %x %x\n", ptr, mem2chunk(ptr), mem2chunk(ptr)->pad, __builtin_cheri_perms_get(mem2chunk(ptr)->pad), CHERI_PERM_CHERIABI_VMMAP);
	if (ptr != mem2chunk(ptr)->pad ||
	    (__builtin_cheri_perms_get(mem2chunk(ptr)->pad) &
	     CHERI_PERM_CHERIABI_VMMAP) == 0)
		USAGE_ERROR_ACTION(m, mem);
#endif
	return ptr;
}
#endif

#ifndef __CHERI__

#ifndef MAP_CHERI_NOSETBOUNDS
#define MAP_CHERI_NOSETBOUNDS   0x0
#endif /*  MAP_CHERI_NOSETBOUNDS  */

#endif /*  ! __CHERI__  */

/* -------------------------- Debugging setup ---------------------------- */

#if ! DEBUG

#define check_free_chunk(M,P)
#define check_inuse_chunk(M,P)
#define check_malloced_chunk(M,P,N)
#define check_mmapped_chunk(M,P)
#define check_malloc_state(M)
#define check_top_chunk(M,P)
#define check_freebuf_corrupt(M,P)

#else /* DEBUG */
#define check_free_chunk(M,P)       do_check_free_chunk(M,P)
#define check_inuse_chunk(M,P)      do_check_inuse_chunk(M,P)
#define check_top_chunk(M,P)        do_check_top_chunk(M,P)
#define check_malloced_chunk(M,P,N) do_check_malloced_chunk(M,P,N)
#define check_mmapped_chunk(M,P)    do_check_mmapped_chunk(M,P)
#define check_malloc_state(M)       do_check_malloc_state(M)
#define check_freebuf_corrupt(M,P)  do_check_freebuf_corrupt(M,P)

static void   do_check_any_chunk(mstate m, mchunkptr p);
static void   do_check_top_chunk(mstate m, mchunkptr p);
static void   do_check_mmapped_chunk(mstate m, mchunkptr p);
static void   do_check_inuse_chunk(mstate m, mchunkptr p);
static void   do_check_free_chunk(mstate m, mchunkptr p);
static void   do_check_malloced_chunk(mstate m, void* mem, size_t s);
static void   do_check_tree(mstate m, tchunkptr t);
static void   do_check_treebin(mstate m, bindex_t i);
static void   do_check_smallbin(mstate m, bindex_t i);
static void   do_check_malloc_state(mstate m);
static int    bin_find(mstate m, mchunkptr x);
static size_t traverse_and_check(mstate m);
#endif /* DEBUG */

/* ---------------------------- Indexing Bins ---------------------------- */

#define is_small(s)         (((s) >> SMALLBIN_SHIFT) < NSMALLBINS)
#define small_index(s)      (bindex_t)((s)  >> SMALLBIN_SHIFT)
#define small_index2size(i) ((i)  << SMALLBIN_SHIFT)
#define MIN_SMALL_INDEX     (small_index(MIN_CHUNK_SIZE))

/* addressing by index. See above about smallbin repositioning */
#define smallbin_at(M, i)   ((sbinptr)((char*)&((M)->smallbins[(i)<<1])))
#define treebin_at(M,i)     (&((M)->treebins[i]))

/* assign tree index for size S to variable I. Use x86 asm if possible  */
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define compute_tree_index(S, I)\
{\
  unsigned int X = S >> TREEBIN_SHIFT;\
  if (X == 0)\
    I = 0;\
  else if (X > 0xFFFF)\
    I = NTREEBINS-1;\
  else {\
    unsigned int K = (unsigned) sizeof(X)*__CHAR_BIT__ - 1 - (unsigned) __builtin_clz(X); \
    I =  (bindex_t)((K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1)));\
  }\
}

#elif defined (__INTEL_COMPILER)
#define compute_tree_index(S, I)\
{\
  size_t X = S >> TREEBIN_SHIFT;\
  if (X == 0)\
    I = 0;\
  else if (X > 0xFFFF)\
    I = NTREEBINS-1;\
  else {\
    unsigned int K = _bit_scan_reverse (X); \
    I =  (bindex_t)((K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1)));\
  }\
}

#else /* GNUC */
#define compute_tree_index(S, I)\
{\
  size_t X = S >> TREEBIN_SHIFT;\
  if (X == 0)\
    I = 0;\
  else if (X > 0xFFFF)\
    I = NTREEBINS-1;\
  else {\
    unsigned int Y = (unsigned int)X;\
    unsigned int N = ((Y - 0x100) >> 16) & 8;\
    unsigned int K = (((Y <<= N) - 0x1000) >> 16) & 4;\
    N += K;\
    N += K = (((Y <<= K) - 0x4000) >> 16) & 2;\
    K = 14 - N + ((Y <<= K) >> 15);\
    I = (K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1));\
  }\
}
#endif /* GNUC */

/* Bit representing maximum resolved size in a treebin at i */
#define bit_for_tree_index(i) \
   (i == NTREEBINS-1)? (SIZE_T_BITSIZE-1) : (((i) >> 1) + TREEBIN_SHIFT - 2)

/* Shift placing maximum resolved bit in a treebin at i as sign bit */
#define leftshift_for_tree_index(i) \
   ((i == NTREEBINS-1)? 0 : \
    ((SIZE_T_BITSIZE-SIZE_T_ONE) - (((i) >> 1) + TREEBIN_SHIFT - 2)))

/* The size of the smallest chunk held in bin with index i */
#define minsize_for_tree_index(i) \
   ((SIZE_T_ONE << (((i) >> 1) + TREEBIN_SHIFT)) |  \
   (((size_t)((i) & SIZE_T_ONE)) << (((i) >> 1) + TREEBIN_SHIFT - 1)))


/* ------------------------ Operations on bin maps ----------------------- */

/* bit corresponding to given index */
#define idx2bit(i)              ((binmap_t)(1) << (i))

/* Mark/Clear bits with given index */
#define mark_smallmap(M,i)      ((M)->smallmap |=  idx2bit(i))
#define clear_smallmap(M,i)     ((M)->smallmap &= ~idx2bit(i))
#define smallmap_is_marked(M,i) ((M)->smallmap &   idx2bit(i))

#define mark_treemap(M,i)       ((M)->treemap  |=  idx2bit(i))
#define clear_treemap(M,i)      ((M)->treemap  &= ~idx2bit(i))
#define treemap_is_marked(M,i)  ((M)->treemap  &   idx2bit(i))

/* isolate the least set bit of a bitmap */
#define least_bit(x)         ((x) & -(x))

/* mask with all bits to left of least bit of x on */
#define left_bits(x)         ((x<<1) | -(x<<1))

/* mask with all bits to left of or equal to least bit of x on */
#define same_or_left_bits(x) ((x) | -(x))

/* index corresponding to given bit. Use x86 asm if possible */

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define compute_bit2idx(X, I)\
{\
  unsigned int J;\
  J = __builtin_ctz(X); \
  I = (bindex_t)J;\
}

#elif defined (__INTEL_COMPILER)
#define compute_bit2idx(X, I)\
{\
  unsigned int J;\
  J = _bit_scan_forward (X); \
  I = (bindex_t)J;\
}

#elif USE_BUILTIN_FFS
#define compute_bit2idx(X, I) I = ffs(X)-1

#else
#define compute_bit2idx(X, I)\
{\
  unsigned int Y = X - 1;\
  unsigned int K = Y >> (16-4) & 16;\
  unsigned int N = K;        Y >>= K;\
  N += K = Y >> (8-3) &  8;  Y >>= K;\
  N += K = Y >> (4-2) &  4;  Y >>= K;\
  N += K = Y >> (2-1) &  2;  Y >>= K;\
  N += K = Y >> (1-0) &  1;  Y >>= K;\
  I = (bindex_t)(N + Y);\
}
#endif /* GNUC */


/* ----------------------- Runtime Check Support ------------------------- */

/*
  For security, the main invariant is that malloc/free/etc never
  writes to a static address other than malloc_state, unless static
  malloc_state itself has been corrupted, which cannot occur via
  malloc (because of these checks). In essence this means that we
  believe all pointers, sizes, maps etc held in malloc_state, but
  check all of those linked or offsetted from other embedded data
  structures.  These checks are interspersed with main code in a way
  that tends to minimize their run-time cost.

  When FOOTERS is defined, in addition to range checking, we also
  verify footer fields of inuse chunks, which can be used guarantee
  that the mstate controlling malloc/free is intact.  This is a
  streamlined version of the approach described by William Robertson
  et al in "Run-time Detection of Heap-based Overflows" LISA'03
  http://www.usenix.org/events/lisa03/tech/robertson.html The footer
  of an inuse chunk holds the xor of its mstate and a random seed,
  that is checked upon calls to free() and realloc().  This is
  (probabalistically) unguessable from outside the program, but can be
  computed by any code successfully malloc'ing any chunk, so does not
  itself provide protection against code that has already broken
  security through some other means.  Unlike Robertson et al, we
  always dynamically check addresses of all offset chunks (previous,
  next, etc). This turns out to be cheaper than relying on hashes.
*/

#if !INSECURE
/* Check if address a is at least as high as any from MMAP */
#define ok_address(M, a) ((char*)(a) >= (M)->least_addr)
/* Check if address of next chunk n is higher than base chunk p */
#define ok_next(p, n)    ((char*)(p) < (char*)(n))
/* Check if p has inuse status */
#define ok_inuse(p)     is_inuse(p)
/* Check if p has its pinuse bit on */
#define ok_pinuse(p)     pinuse(p)

#else /* !INSECURE */
#define ok_address(M, a) (1)
#define ok_next(b, n)    (1)
#define ok_inuse(p)      (1)
#define ok_pinuse(p)     (1)
#endif /* !INSECURE */

#define ok_magic(M)      (1)

/* In gcc, use __builtin_expect to minimize impact of checks */
#if !INSECURE
#if defined(__GNUC__) && __GNUC__ >= 3
#define RTCHECK(e)  __builtin_expect(e, 1)
#else /* GNUC */
#define RTCHECK(e)  (e)
#endif /* GNUC */
#else /* !INSECURE */
#define RTCHECK(e)  (1)
#endif /* !INSECURE */

/* macros to set up inuse chunks with or without footers */


#define mark_inuse_foot(M,p,s)

/* Macros for setting head/foot of non-mmapped chunks */

/* Set cinuse bit and pinuse bit of next chunk */
#define set_inuse(M,p,s)\
  ((p)->head = (pdirty(p)|((p)->head & PINUSE_BIT)|s|CINUSE_BIT),\
  ((mchunkptr)(((char*)(p)) + (s)))->head |= PINUSE_BIT, clear_pdirty((mchunkptr)(((char*)(p)) + (s))))

/* Set cinuse and pinuse of this chunk and pinuse of next chunk */
#define set_inuse_and_pinuse(M,p,s)\
  ((p)->head = (pdirty(p)|s|PINUSE_BIT|CINUSE_BIT),\
  ((mchunkptr)(((char*)(p)) + (s)))->head |= PINUSE_BIT, clear_pdirty((mchunkptr)(((char*)(p)) + (s))))

/* Set size, cinuse and pinuse bit of this chunk */
#define set_size_and_pinuse_of_inuse_chunk(M, p, s)\
  ((p)->head = (pdirty(p)|s|PINUSE_BIT|CINUSE_BIT))


/* ---------------------------- setting mparams -------------------------- */

#if LOCK_AT_FORK
static void pre_fork(void)         { ACQUIRE_LOCK(&(gm)->mutex); }
static void post_fork_parent(void) { RELEASE_LOCK(&(gm)->mutex); }
static void post_fork_child(void)  { INITIAL_LOCK(&(gm)->mutex); }
#endif /* LOCK_AT_FORK */

/* Initialize mparams */
static int init_mparams(void) {
  const char *valuestr;
  char *endp;
  ssize_t value;

  ACQUIRE_MALLOC_GLOBAL_LOCK();
  if (mparams.magic == 0) {
    size_t magic;
    size_t psize;
    size_t gsize;

    UTRACE(0, 0, 0);

    psize = malloc_getpagesize;
    gsize = ((DEFAULT_GRANULARITY != 0)? DEFAULT_GRANULARITY : psize);

    /* Sanity-check configuration:
       ints must be at least 4 bytes.
       alignment must be at least 32.
       Alignment, min chunk size, and page size must all be powers of 2.
    */
    if ((MAX_SIZE_T < MIN_CHUNK_SIZE)  ||
        (sizeof(int) < 4)  ||
        (MALLOC_ALIGNMENT < (size_t)32U) ||
        ((MALLOC_ALIGNMENT & (MALLOC_ALIGNMENT-SIZE_T_ONE)) != 0) ||
        ((MCHUNK_SIZE      & (MCHUNK_SIZE-SIZE_T_ONE))      != 0) ||
        ((gsize            & (gsize-SIZE_T_ONE))            != 0) ||
        ((psize            & (psize-SIZE_T_ONE))            != 0))
      ABORT;
    mparams.granularity = gsize;
    mparams.page_size = psize;
    mparams.mmap_threshold = DEFAULT_MMAP_THRESHOLD;
    mparams.trim_threshold = DEFAULT_TRIM_THRESHOLD;
    mparams.default_mflags = USE_LOCK_BIT|USE_MMAP_BIT|USE_NONCONTIGUOUS_BIT|DO_REVOCATION_BIT;

#if 0
    if ((valuestr = getenv("MALLOC_MAX_FREEBUF_BYTES")) != NULL) {
      value = strtol(valuestr, &endp, 0);
      if (value == 0 && *endp != '\0')
        value = DEFAULT_MAX_FREEBUFBYTES;
      if (value < 0)
        value = MAX_SIZE_T;
      mparams.max_freebufbytes = value;
    } else
#endif
      mparams.max_freebufbytes = DEFAULT_MAX_FREEBUFBYTES;

#if 0
    if ((valuestr = getenv("MALLOC_MIN_FREEBUF_BYTES")) != NULL) {
      value = strtol(valuestr, &endp, 0);
      if (value == 0 && *endp != '\0')
        value = DEFAULT_MIN_FREEBUFBYTES;
      if (value < 0)
        value = MAX_SIZE_T;
      mparams.min_freebufbytes = value;
    } else
#endif
      mparams.min_freebufbytes = DEFAULT_MIN_FREEBUFBYTES;

    mparams.max_freebuf_percent = DEFAULT_FREEBUF_PERCENT;
#if 0
    if ((valuestr = getenv("MALLOC_MAX_FREEBUF_PERCENT")) != NULL) {
      value = strtol(valuestr, &endp, 0);
      if (value < 0)
	value = MAX_SIZE_T;
      if (value != 0 || *endp == '\0')
        mparams.max_freebuf_percent = (double)value / 100.0;
    }
#endif
#if SUPPORT_UNMAP
    if ((valuestr = getenv("MALLOC_UNMAP_THRESHOLD")) != NULL) {
      value = strtol(valuestr, &endp, 0);
      if (value == 0 && *endp != '\0')
	value = DEFAULT_UNMAP_THRESHOLD;
      if (value < 0 || value > MAX_UNMAP_THRESHOLD)
	value = MAX_UNMAP_THRESHOLD;
      mparams.unmap_threshold = psize * value;
    } else
      mparams.unmap_threshold = psize * DEFAULT_UNMAP_THRESHOLD;
#endif

    /* Set up lock for main malloc area */
    gm->mflags = mparams.default_mflags;
    (void)INITIAL_LOCK(&gm->mutex);
#if LOCK_AT_FORK
    pthread_atfork(&pre_fork, &post_fork_parent, &post_fork_child);
#endif

    /*
     * Allow revocation to be disabled if MALLOC_DISABLE_REVOCATION is
     * defined in the environment.  Don't allow if we're obviously SUID.
     */
#if 0
    if(getenv("MALLOC_DISABLE_REVOCATION") != NULL &&
       getuid() == geteuid())
      disable_revocation(gm);
#endif
    {
#if USE_DEV_RANDOM
      int fd;
      unsigned char buf[sizeof(size_t)];
      /* Try to use /dev/urandom, else fall back on using time */
      if ((fd = open("/dev/urandom", O_RDONLY)) >= 0 &&
          read(fd, buf, sizeof(buf)) == sizeof(buf)) {
        magic = *((size_t *) buf);
        close(fd);
      }
      else
#endif /* USE_DEV_RANDOM */
#if defined(LACKS_TIME_H)
      magic = (size_t)&magic ^ (size_t)0x55555555U;
#else
      magic = (size_t)(time(0) ^ (size_t)0x55555555U);
#endif
      magic |= (size_t)8U;    /* ensure nonzero */
      magic &= ~(size_t)7U;   /* improve chances of fault for bad values */
      /* Until memory modes commonly available, use volatile-write */
      (*(volatile size_t *)(&(mparams.magic))) = magic;
    }
  }

  RELEASE_MALLOC_GLOBAL_LOCK();
  return 1;
}

/* support for mallopt */
static int change_mparam(int param_number, ssize_t value) {
  size_t val;
  ensure_initialization();
  val = (value == -1)? MAX_SIZE_T : (size_t)value;
  switch(param_number) {
  case M_TRIM_THRESHOLD:
    mparams.trim_threshold = val;
    return 1;
  case M_GRANULARITY:
    if (val >= mparams.page_size && ((val & (val-1)) == 0)) {
      mparams.granularity = val;
      return 1;
    }
    else
      return 0;
  case M_MMAP_THRESHOLD:
    mparams.mmap_threshold = val;
    return 1;
  case M_MIN_FREEBUFBYTES:
    mparams.min_freebufbytes = val;
    return 1;
  case M_MAX_FREEBUFBYTES:
    mparams.max_freebufbytes = val;
    return 1;
  case M_MAX_FREEBUF_PERCENT:
    /* Values over 100 are all infinity */
    mparams.max_freebuf_percent = val / 100.0;
    return 1;
#if SUPPORT_UNMAP
  case M_UNMAP_THRESHOLD:
    mparams.unmap_threshold = value < 0 || value > MAX_UNMAP_THRESHOLD ?
	MAX_UNMAP_THRESHOLD : (size_t)value;
#endif
  default:
    return 0;
  }
}

#if DEBUG
/* ------------------------- Debugging Support --------------------------- */

static void
do_check_freebuf_corrupt(mstate m, mchunkptr p) {
  mchunkptr freebinptr = &m->freebufbin;
  /*
   * XXX: before anything has been free'd, first freebin entry points to
   * NULL.  Perhaps more initilization is required...
   */
  if (freebinptr->fd->fd == NULL)
    return;
  for(mchunkptr iter=freebinptr->fd; iter!=freebinptr; iter=iter->fd) {
    assert(p != iter);
  }
}

/* Check properties of any chunk, whether free, inuse, mmapped etc  */
static void do_check_any_chunk(mstate m, mchunkptr p) {
  assert((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD));
  assert(ok_address(m, p));
}

/* Check properties of top chunk */
static void do_check_top_chunk(mstate m, mchunkptr p) {
  msegmentptr sp = segment_holding(m, (char*)p);
  size_t  sz = p->head & ~FLAG_BITS; /* third-lowest bit can be set! */
  assert(sp != 0);
  assert((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD));
  assert(ok_address(m, p));
  assert(sz == m->topsize);
  assert(sz > 0);
  assert(sz == ((sp->base + sp->size) - (char*)p) - TOP_FOOT_SIZE);
  assert(pinuse(p));
  assert(!pinuse(chunk_plus_offset(p, sz)));
  assert(!cdirty(p));
}

/* Check properties of (inuse) mmapped chunks */
static void do_check_mmapped_chunk(mstate m, mchunkptr p) {
  size_t  sz = chunksize(p);
  size_t len = (sz + (p->prev_foot) + MMAP_FOOT_PAD);
  assert(is_mmapped(p));
  assert(use_mmap(m));
  assert((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD));
  assert(ok_address(m, p));
  assert(!is_small(sz));
  assert((len & (mparams.page_size-SIZE_T_ONE)) == 0);
  assert(chunk_plus_offset(p, sz)->head == FENCEPOST_HEAD);
  assert(chunk_plus_offset(p, sz+SIZE_T_SIZE)->head == 0);
}

/* Check properties of inuse chunks */
static void do_check_inuse_chunk(mstate m, mchunkptr p) {
  do_check_any_chunk(m, p);
  assert(is_inuse(p));
  assert(next_pinuse(p));
  /* If not pinuse and not mmapped, previous chunk has OK offset */
  assert(is_mmapped(p) || pinuse(p) || next_chunk(prev_chunk(p)) == p);
  if (is_mmapped(p))
    do_check_mmapped_chunk(m, p);
}

/* Check properties of free chunks */
static void do_check_free_chunk(mstate m, mchunkptr p) {
  size_t sz = chunksize(p);
  mchunkptr next = chunk_plus_offset(p, sz);
  do_check_any_chunk(m, p);
  assert(!is_inuse(p));
  assert(!next_pinuse(p));
  assert (!is_mmapped(p));
  if (p != m->dv && p != m->top) {
    if (sz >= MIN_CHUNK_SIZE) {
      assert((sz & CHUNK_ALIGN_MASK) == 0);
      assert(is_aligned(chunk2mem(p)));
      assert(next->prev_foot == sz);
      assert(pinuse(p));
      assert (next == m->top || is_inuse(next));
      assert(p->fd->bk == p);
      assert(p->bk->fd == p);
    }
    else  /* markers are always of size SIZE_T_SIZE */
      assert(sz == SIZE_T_SIZE);
  }
}

/* Check properties of malloced chunks at the point they are malloced */
static void do_check_malloced_chunk(mstate m, void* mem, size_t s) {
  if (mem != 0) {
    mchunkptr p = mem2chunk(mem);
    size_t sz = p->head & ~FLAG_BITS;
    do_check_inuse_chunk(m, p);
    assert((sz & CHUNK_ALIGN_MASK) == 0);
    assert(sz >= MIN_CHUNK_SIZE);
    assert(sz >= s);
    /* unless mmapped, size is less than MIN_CHUNK_SIZE more than request */
    assert(is_mmapped(p) || sz < (s + MIN_CHUNK_SIZE));
  }
}

/* Check a tree and its subtrees.  */
static void do_check_tree(mstate m, tchunkptr t) {
  tchunkptr head = 0;
  tchunkptr u = t;
  bindex_t tindex = t->index;
  size_t tsize = chunksize(t);
  bindex_t idx;
  compute_tree_index(tsize, idx);
  assert(tindex == idx);
  assert(tsize >= MIN_LARGE_SIZE);
  assert(tsize >= minsize_for_tree_index(idx));
  assert((idx == NTREEBINS-1) || (tsize < minsize_for_tree_index((idx+1))));

  do { /* traverse through chain of same-sized nodes */
    do_check_any_chunk(m, ((mchunkptr)u));
    assert(u->index == tindex);
    assert(chunksize(u) == tsize);
    assert(!is_inuse(u));
    assert(!next_pinuse(u));
    assert(u->fd->bk == u);
    assert(u->bk->fd == u);
    if (u->parent == 0) {
      assert(u->child[0] == 0);
      assert(u->child[1] == 0);
    }
    else {
      assert(head == 0); /* only one node on chain has parent */
      head = u;
      assert(u->parent != u);
      assert (u->parent->child[0] == u ||
              u->parent->child[1] == u ||
              *((tbinptr*)(u->parent)) == u);
      if (u->child[0] != 0) {
        assert(u->child[0]->parent == u);
        assert(u->child[0] != u);
        do_check_tree(m, u->child[0]);
      }
      if (u->child[1] != 0) {
        assert(u->child[1]->parent == u);
        assert(u->child[1] != u);
        do_check_tree(m, u->child[1]);
      }
      if (u->child[0] != 0 && u->child[1] != 0) {
        assert(chunksize(u->child[0]) < chunksize(u->child[1]));
      }
    }
    u = u->fd;
  } while (u != t);
  assert(head != 0);
}

/*  Check all the chunks in a treebin.  */
static void do_check_treebin(mstate m, bindex_t i) {
  tbinptr* tb = treebin_at(m, i);
  tchunkptr t = *tb;
  int empty = (m->treemap & (1U << i)) == 0;
  if (t == 0)
    assert(empty);
  if (!empty)
    do_check_tree(m, t);
}

/*  Check all the chunks in a smallbin.  */
static void do_check_smallbin(mstate m, bindex_t i) {
  sbinptr b = smallbin_at(m, i);
  mchunkptr p = b->bk;
  unsigned int empty = (m->smallmap & (1U << i)) == 0;
  if (p == b)
    assert(empty);
  if (!empty) {
    for (; p != b; p = p->bk) {
      size_t size = chunksize(p);
      mchunkptr q;
      /* each chunk claims to be free */
      do_check_free_chunk(m, p);
      /* chunk belongs in bin */
      assert(small_index(size) == i);
      assert(p->bk == b || chunksize(p->bk) == chunksize(p));
      /* chunk is followed by an inuse chunk */
      q = next_chunk(p);
      if (q->head != FENCEPOST_HEAD)
        do_check_inuse_chunk(m, q);
    }
  }
}

/* Find x in a bin. Used in other check functions. */
static int bin_find(mstate m, mchunkptr x) {
  size_t size = chunksize(x);
  if (is_small(size)) {
    bindex_t sidx = small_index(size);
    sbinptr b = smallbin_at(m, sidx);
    if (smallmap_is_marked(m, sidx)) {
      mchunkptr p = b;
      do {
        if (p == x)
          return 1;
      } while ((p = p->fd) != b);
    }
  }
  else {
    bindex_t tidx;
    compute_tree_index(size, tidx);
    if (treemap_is_marked(m, tidx)) {
      tchunkptr t = *treebin_at(m, tidx);
      size_t sizebits = size << leftshift_for_tree_index(tidx);
      while (t != 0 && chunksize(t) != size) {
        t = t->child[(sizebits >> (SIZE_T_BITSIZE-SIZE_T_ONE)) & 1];
        sizebits <<= 1;
      }
      if (t != 0) {
        tchunkptr u = t;
        do {
          if (u == (tchunkptr)x)
            return 1;
        } while ((u = u->fd) != t);
      }
    }
  }
  return 0;
}

/* Traverse each chunk and check it; return total */
static size_t traverse_and_check(mstate m) {
  size_t sum = 0;
  if (is_initialized(m)) {
    msegmentptr s = &m->seg;
    sum += m->topsize + TOP_FOOT_SIZE;
    while (s != 0) {
      mchunkptr q = align_as_chunk(s->base);
      mchunkptr lastq = 0;
      assert(pinuse(q));
      while (segment_holds(s, q) &&
             q != m->top && q->head != FENCEPOST_HEAD) {
        sum += chunksize(q);
        if (is_inuse(q)) {
          assert(!bin_find(m, q));
          do_check_inuse_chunk(m, q);
        }
        else {
          assert(q == m->dv || bin_find(m, q));
          assert(lastq == 0 || is_inuse(lastq)); /* Not 2 consecutive free */
          do_check_free_chunk(m, q);
        }
        lastq = q;
        q = next_chunk(q);
      }
      s = s->next;
    }
  }
  return sum;
}


/* Check all properties of malloc_state. */
static void do_check_malloc_state(mstate m) {
  bindex_t i;
  size_t total;
  /* check bins */
  for (i = 0; i < NSMALLBINS; ++i)
    do_check_smallbin(m, i);
  for (i = 0; i < NTREEBINS; ++i)
    do_check_treebin(m, i);

  if (m->dvsize != 0) { /* check dv chunk */
    do_check_any_chunk(m, m->dv);
    assert(m->dvsize == chunksize(m->dv));
    assert(m->dvsize >= MIN_CHUNK_SIZE);
    assert(bin_find(m, m->dv) == 0);
  }

  if (m->top != 0) {   /* check top chunk */
    do_check_top_chunk(m, m->top);
    /*assert(m->topsize == chunksize(m->top)); redundant */
    assert(m->topsize > 0);
    assert(bin_find(m, m->top) == 0);
  }

  total = traverse_and_check(m);
  assert(total <= m->footprint);
  assert(m->footprint <= m->max_footprint);
}
#endif /* DEBUG */

/* ----------------------------- statistics ------------------------------ */

#if !NO_MALLINFO
static struct mallinfo internal_mallinfo(mstate m) {
  struct mallinfo nm = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  ensure_initialization();
  if (!PREACTION(m)) {
    check_malloc_state(m);
    if (is_initialized(m)) {
      size_t nfree = SIZE_T_ONE; /* top always free */
      size_t mfree = m->topsize + TOP_FOOT_SIZE;
      size_t sum = mfree;
      msegmentptr s = &m->seg;
      while (s != 0) {
        mchunkptr q = align_as_chunk(s->base);
        while (segment_holds(s, q) &&
               q != m->top && q->head != FENCEPOST_HEAD) {
          size_t sz = chunksize(q);
          sum += sz;
          if (!is_inuse(q)) {
            mfree += sz;
            ++nfree;
          }
          q = next_chunk(q);
        }
        s = s->next;
      }

      nm.arena    = sum;
      nm.ordblks  = nfree;
      nm.hblkhd   = m->footprint - sum;
      nm.usmblks  = m->max_footprint;
      nm.uordblks = m->footprint - mfree;
      nm.fordblks = mfree;
      nm.keepcost = m->topsize;
    }

    POSTACTION(m);
  }
  return nm;
}
#endif /* !NO_MALLINFO */

#if !NO_MALLOC_STATS
static void internal_malloc_stats(mstate m) {
  ensure_initialization();
  if (!PREACTION(m)) {
    size_t allocated = 0;
    size_t max_allocated = 0;
    size_t maxfp = 0;
    size_t fp = 0;
    size_t used = 0;
    check_malloc_state(m);
    if (is_initialized(m)) {
      msegmentptr s = &m->seg;
      maxfp = m->max_footprint;
      allocated = m->allocated;
      max_allocated = m->max_allocated;
      fp = m->footprint;
      used = fp - (m->topsize + TOP_FOOT_SIZE);

      while (s != 0) {
        mchunkptr q = align_as_chunk(s->base);
        while (segment_holds(s, q) &&
               q != m->top && q->head != FENCEPOST_HEAD) {
          if (!is_inuse(q))
            used -= chunksize(q);
          mchunkptr next = next_chunk(q);
          // FIXME: work around infinite loop
          if (next == q) {
            malloc_printf("ERROR: Infinite loop in %s: q=%#p, head=0x%zx, m->top=%#p\n",
                          __func__, q, q->head, m->top);
            break;
          }
          q = next;
        }
        s = s->next;
      }
    }
    POSTACTION(m); /* drop lock */
    malloc_printf("max system bytes      = %10zu\n", maxfp);
    malloc_printf("system bytes          = %10zu\n", fp);
    malloc_printf("in use bytes          = %10zu\n", used);
    malloc_printf("allocated bytes       = %10zu\n", allocated);
    malloc_printf("max allocated bytes   = %10zu\n", max_allocated);
#if USE_LOCKS && USE_SPIN_LOCKS && (!defined(USE_RECURSIVE_LOCKS) || USE_RECURSIVE_LOCKS==0)
    malloc_printf("global lock contended = %10zu\n", lockContended);
#endif // USE_LOCKS
  }
}
#endif /* NO_MALLOC_STATS */

/* ----------------------- Operations on smallbins ----------------------- */

/*
  Various forms of linking and unlinking are defined as macros.  Even
  the ones for trees, which are very long but have very short typical
  paths.  This is ugly but reduces reliance on inlining support of
  compilers.
*/

// Link a free chunk into a freebufbin.
#define insert_freebuf_chunk(M, P) {\
  mchunkptr B = &M->freebufbin;\
  mchunkptr L = B;\
  if(RTCHECK(ok_address(M, B->bk) || B->bk==&M->freebufbin))\
    L = B->bk;\
  else {\
    CORRUPTION_ERROR_ACTION(M);\
  }\
  B->bk = P;\
  L->fd = P;\
  P->bk = L;\
  P->fd = B;\
  M->freebufbytes += chunksize(P);\
  M->freebufsize++;\
}

// Unlink a chunk from freebufbin.
#define unlink_freebuf_chunk(M, P) {\
  mchunkptr F = P->fd;\
  mchunkptr B = P->bk;\
  assert(P != B);\
  assert(P != F);\
  if(RTCHECK((ok_address(M, F) || F==&M->freebufbin) && F->bk == P)) { \
    if(RTCHECK((ok_address(M, B) || B==&M->freebufbin) && B->fd == P)) {\
      F->bk = B;\
      B->fd = F;\
      M->freebufbytes -= chunksize(P);\
      M->freebufsize--;\
    } else {\
      CORRUPTION_ERROR_ACTION(M);\
    }\
  } else {\
    CORRUPTION_ERROR_ACTION(M);\
  }\
}

// Unlink the first chunk from freebufbin.
#define unlink_first_freebuf_chunk(M, B, P) {\
  mchunkptr F = P->fd;\
  assert(P != B);\
  assert(P != F);\
  if(RTCHECK((ok_address(M, F) || F==&M->freebufbin) && F->bk == P)) {\
    F->bk = B;\
    B->fd = F;\
    M->freebufbytes -= chunksize(P);\
    M->freebufsize--;\
  } else {\
    CORRUPTION_ERROR_ACTION(M);\
  }\
}

/* Link a free chunk into a smallbin  */
static void insert_small_chunk(mstate M, mchunkptr P, size_t S) {
  bindex_t I  = small_index(S);
  mchunkptr B = smallbin_at(M, I);
  mchunkptr F = B;
  assert(S >= MIN_CHUNK_SIZE);
  if (!smallmap_is_marked(M, I))
    mark_smallmap(M, I);
  else if (RTCHECK(ok_address(M, B->fd)))
    F = B->fd;
  else {
    CORRUPTION_ERROR_ACTION(M);
  }
  B->fd = P;
  F->bk = P;
  P->fd = F;
  P->bk = B;
}

/* Unlink a chunk from a smallbin  */
static void unlink_small_chunk(mstate M, mchunkptr P, size_t S) {
  mchunkptr F = P->fd;
  mchunkptr B = P->bk;
  bindex_t I = small_index(S);
  assert(P != B);
  assert(P != F);
  assert(chunksize(P) == small_index2size(I));
  if (RTCHECK(F == smallbin_at(M,I) || (ok_address(M, F) && F->bk == P))) { 
    if (B == F) {
      clear_smallmap(M, I);
    }
    else if (RTCHECK(B == smallbin_at(M,I) ||
                     (ok_address(M, B) && B->fd == P))) {
      F->bk = B;
      B->fd = F;
    }
    else {
      CORRUPTION_ERROR_ACTION(M);
    }
  }
  else {
    CORRUPTION_ERROR_ACTION(M);
  }
}

/* Unlink the first chunk from a smallbin */
static void unlink_first_small_chunk(mstate M, mchunkptr B, mchunkptr P,
    bindex_t I) {
  mchunkptr F = P->fd;
  assert(P != B);
  assert(P != F);
  assert(chunksize(P) == small_index2size(I));
  if (B == F) {
    clear_smallmap(M, I);
  }
  else if (RTCHECK(ok_address(M, F) && F->bk == P)) {
    F->bk = B;
    B->fd = F;
  }
  else {
    CORRUPTION_ERROR_ACTION(M);
  }
}

/* Replace dv node, binning the old one */
/* Used only when dvsize known to be small */
#define replace_dv(M, P, S) {\
  size_t DVS = M->dvsize;\
  assert(is_small(DVS));\
  if (DVS != 0) {\
    mchunkptr DV = M->dv;\
    insert_small_chunk(M, DV, DVS);\
  }\
  M->dvsize = S;\
  M->dv = P;\
}

/* ------------------------- Operations on trees ------------------------- */

/* Insert chunk into tree */
static void insert_large_chunk(mstate M, tchunkptr X, size_t S) {
  tbinptr* H;
  bindex_t I;
  compute_tree_index(S, I);
  H = treebin_at(M, I);
  X->index = I;
  X->child[0] = X->child[1] = 0;
  if (!treemap_is_marked(M, I)) {
    mark_treemap(M, I);
    *H = X;
    X->parent = (tchunkptr)H;
    X->fd = X->bk = X;
  }
  else {
    tchunkptr T = *H;
    size_t K = S << leftshift_for_tree_index(I);
    for (;;) {
      if (chunksize(T) != S) {
        tchunkptr* C = &(T->child[(K >> (SIZE_T_BITSIZE-SIZE_T_ONE)) & 1]);
        K <<= 1;
        if (*C != 0)
          T = *C;
        else if (RTCHECK(ok_address(M, C))) {
          *C = X;
          X->parent = T;
          X->fd = X->bk = X;
          break;
        }
        else {
          CORRUPTION_ERROR_ACTION(M);
          break;
        }
      }
      else {
        tchunkptr F = T->fd;
        if (RTCHECK(ok_address(M, T) && ok_address(M, F))) {
          T->fd = F->bk = X;
          X->fd = F;
          X->bk = T;
          X->parent = 0;
          break;
        }
        else {
          CORRUPTION_ERROR_ACTION(M);
          break;
        }
      }
    }
  }
}

/*
  Unlink steps:

  1. If x is a chained node, unlink it from its same-sized fd/bk links
     and choose its bk node as its replacement.
  2. If x was the last node of its size, but not a leaf node, it must
     be replaced with a leaf node (not merely one with an open left or
     right), to make sure that lefts and rights of descendents
     correspond properly to bit masks.  We use the rightmost descendent
     of x.  We could use any other leaf, but this is easy to locate and
     tends to counteract removal of leftmosts elsewhere, and so keeps
     paths shorter than minimally guaranteed.  This doesn't loop much
     because on average a node in a tree is near the bottom.
  3. If x is the base of a chain (i.e., has parent links) relink
     x's parent and children to x's replacement (or null if none).
*/

static void unlink_large_chunk(mstate M, tchunkptr X) {
  tchunkptr XP = X->parent;
  tchunkptr R;
  if (X->bk != X) {
    tchunkptr F = X->fd;
    R = X->bk;
    if (RTCHECK(ok_address(M, F) && F->bk == X && R->fd == X)) {
      F->bk = R;
      R->fd = F;
    }
    else {
      CORRUPTION_ERROR_ACTION(M);
    }
  }
  else {
    tchunkptr* RP;
    if (((R = *(RP = &(X->child[1]))) != 0) ||
        ((R = *(RP = &(X->child[0]))) != 0)) {
      tchunkptr* CP;
      while ((*(CP = &(R->child[1])) != 0) ||
             (*(CP = &(R->child[0])) != 0)) {
        R = *(RP = CP);
      }
      if (RTCHECK(ok_address(M, RP)))
        *RP = 0;
      else {
        CORRUPTION_ERROR_ACTION(M);
      }
    }
  }
  if (XP != 0) {
    tbinptr* H = treebin_at(M, X->index);
    if (X == *H) {
      if ((*H = R) == 0) 
        clear_treemap(M, X->index);
    }
    else if (RTCHECK(ok_address(M, XP))) {
      if (XP->child[0] == X) 
        XP->child[0] = R;
      else 
        XP->child[1] = R;
    }
    else
      CORRUPTION_ERROR_ACTION(M);
    if (R != 0) {
      if (RTCHECK(ok_address(M, R))) {
        tchunkptr C0, C1;
        R->parent = XP;
        if ((C0 = X->child[0]) != 0) {
          if (RTCHECK(ok_address(M, C0))) {
            R->child[0] = C0;
            C0->parent = R;
          }
          else
            CORRUPTION_ERROR_ACTION(M);
        }
        if ((C1 = X->child[1]) != 0) {
          if (RTCHECK(ok_address(M, C1))) {
            R->child[1] = C1;
            C1->parent = R;
          }
          else
            CORRUPTION_ERROR_ACTION(M);
        }
      }
      else
        CORRUPTION_ERROR_ACTION(M);
    }
  }
}

/* Relays to large vs small bin operations */

#define insert_chunk(M, P, S)\
  if (is_small(S)) insert_small_chunk(M, P, S);\
  else { tchunkptr TP = (tchunkptr)(P); insert_large_chunk(M, TP, S); }

#define unlink_chunk(M, P, S)\
  do {\
    if (is_small(S)) unlink_small_chunk(M, P, S);\
    else { tchunkptr TP = (tchunkptr)(P); unlink_large_chunk(M, TP); }\
    clear_meta((P), (S));\
  } while(0)


/* Relays to internal calls to malloc/free from realloc, memalign etc */

#define internal_free(m, mem) dlfree(mem)

/* -----------------------  Direct-mmapping chunks ----------------------- */

/*
  Directly mmapped chunks are set up with an offset to the start of
  the mmapped region stored in the prev_foot field of the chunk. This
  allows reconstruction of the required argument to MUNMAP when freed,
  and also allows adjustment of the returned chunk to meet alignment
  requirements (especially in memalign).
*/

/* Malloc using mmap */
static void* mmap_alloc(mstate m, size_t nb) {
#ifdef __CHERI_PURE_CAPABILITY__
  /* Should never be called */
  ABORT;
#endif
  size_t mmsize = mmap_align(nb + SIX_SIZE_T_SIZES + CHUNK_ALIGN_MASK);
  if (m->footprint_limit != 0) {
    size_t fp = m->footprint + mmsize;
    if (fp <= m->footprint || fp > m->footprint_limit)
      return 0;
  }
  if (mmsize > nb) {     /* Check for wrap around 0 */
    char* mm = (char*)(CALL_DIRECT_MMAP(mmsize));
    if (mm != CMFAIL) {
#if defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__)
      if(MMAP_SHADOW(mm, mmsize) != (void*)((size_t)mm>>MALLOC_ALIGN_BITSHIFT)) {
        ABORT;
      }
#endif   /* defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__) */
      size_t offset = align_offset(chunk2mem(mm));
      size_t psize = mmsize - offset - MMAP_FOOT_PAD;
      mchunkptr p = (mchunkptr)(mm + offset);
      p->prev_foot = offset;
      p->head = psize;
      mark_inuse_foot(m, p, psize);
      chunk_plus_offset(p, psize)->head = FENCEPOST_HEAD;
      chunk_plus_offset(p, psize+SIZE_T_SIZE)->head = 0;

      if (m->least_addr == 0 || mm < m->least_addr)
        m->least_addr = mm;
      if ((m->footprint += mmsize) > m->max_footprint)
        m->max_footprint = m->footprint;
      assert(is_aligned(chunk2mem(p)));
      check_mmapped_chunk(m, p);
      return chunk2mem(p);
    }
  }
  return 0;
}

/* Realloc using mmap */
static mchunkptr mmap_resize(mstate m, mchunkptr oldp, size_t nb) {
  size_t oldsize = chunksize(oldp);
  if (is_small(nb)) /* Can't shrink mmap regions below small size */
    return 0;
  /* Keep old chunk if big enough but not too big */
  if (oldsize >= nb + SIZE_T_SIZE &&
      (oldsize - nb) <= (mparams.granularity << 1))
    return oldp;
  else {
    return 0;
  }
}


/* -------------------------- mspace management -------------------------- */

/* Initialize top chunk and its size */
static void init_top(mstate m, mchunkptr p, size_t psize) {
  /* Ensure alignment */
  size_t offset = align_offset(chunk2mem(p));
  p = (mchunkptr)((char*)p + offset);
  psize -= offset;

  m->top = p;
  m->topsize = psize;
  p->head = dirtybits(p) | psize | PINUSE_BIT;
  /* set size of fake trailing chunk holding overhead space only once */
  chunk_plus_offset(p, psize)->head = TOP_FOOT_SIZE;
  m->trim_check = mparams.trim_threshold; /* reset on each update */
}

/* Initialize bins for a new mstate that is otherwise zeroed out */
static void init_bins(mstate m) {
  /* Establish circular links for smallbins */
  bindex_t i;
  for (i = 0; i < NSMALLBINS; ++i) {
    sbinptr bin = smallbin_at(m,i);
    bin->fd = bin->bk = bin;
  }
  mchunkptr freebin = &m->freebufbin;
  freebin->fd = freebin->bk = freebin;
}

#ifndef __CHERI_PURE_CAPABILITY__
/* Allocate chunk and prepend remainder with chunk in successor base. */
static void* prepend_alloc(mstate m, char* newbase, char* oldbase,
                           size_t nb) {
  mchunkptr p = align_as_chunk(newbase);
  mchunkptr oldfirst = align_as_chunk(oldbase);
  size_t psize = (char*)oldfirst - (char*)p;
  mchunkptr q = chunk_plus_offset(p, nb);
  size_t qsize = psize - nb;
  set_size_and_pinuse_of_inuse_chunk(m, p, nb);

  assert((char*)oldfirst > (char*)q);
  assert(pinuse(oldfirst));
  assert(qsize >= MIN_CHUNK_SIZE);

  /* consolidate remainder with first chunk of old base */
  if (oldfirst == m->top) {
    size_t tsize = m->topsize += qsize;
    m->top = q;
    q->head = tsize | PINUSE_BIT;
    check_top_chunk(m, q);
  }
  else if (oldfirst == m->dv) {
    size_t dsize = m->dvsize += qsize;
    m->dv = q;
    set_size_and_pinuse_of_free_chunk(q, dsize);
  }
  else {
    if (!is_inuse(oldfirst)) {
      size_t nsize = chunksize(oldfirst);
      unlink_chunk(m, oldfirst, nsize);
      oldfirst = chunk_plus_offset(oldfirst, nsize);
      qsize += nsize;
    }
    set_free_with_pinuse(q, qsize, oldfirst);
    insert_chunk(m, q, qsize);
    check_free_chunk(m, q);
  }

  check_malloced_chunk(m, chunk2mem(p), nb);
  return chunk2mem(p);
}
#endif /* !__CHERI_PURE_CAPABILITY__ */

/* Add a segment to hold a new noncontiguous region */
static void add_segment(mstate m, char* tbase, size_t tsize, flag_t mmapped) {
  /* Determine locations and sizes of segment, fenceposts, old top */
  char* old_top = (char*)m->top;
  msegmentptr oldsp = segment_holding(m, old_top);
  char* old_end = oldsp->base + oldsp->size;
  size_t ssize = pad_request(sizeof(struct malloc_segment));
  char* rawsp = old_end - (ssize + CHUNK_HEADER_OFFSET + TWO_SIZE_T_SIZES + CHUNK_ALIGN_MASK);
  size_t offset = align_offset(chunk2mem(rawsp));
  char* asp = rawsp + offset;
  char* csp = (asp < (old_top + MIN_CHUNK_SIZE))? old_top : asp;
  mchunkptr sp = (mchunkptr)csp;
  msegmentptr ss = (msegmentptr)(chunk2mem(sp));
  mchunkptr tnext = chunk_plus_offset(ss, ssize);
  mchunkptr p = tnext;
  int nfences = 0;

  /* reset top to new space */
  init_top(m, (mchunkptr)tbase, tsize - TOP_FOOT_SIZE);

  /* Set up segment record */
  assert(is_aligned(ss));
  set_size_and_pinuse_of_inuse_chunk(m, sp, ssize);
  *ss = m->seg; /* Push current record */
  m->seg.base = tbase;
  m->seg.size = tsize;
  m->seg.sflags = mmapped;
  m->seg.next = ss;
#ifdef CAPREVOKE
  if (do_revocation(m) &&
      caprevoke_shadow(CAPREVOKE_SHADOW_NOVMMAP, tbase, &m->seg.shadow) != 0)
    ABORT;
#endif

  /* Insert trailing fenceposts */
  for (;;) {
    mchunkptr nextp = chunk_plus_offset(p, SIZE_T_SIZE);
    p->head = FENCEPOST_HEAD;
    ++nfences;
    if ((char*)(&(nextp->head)) < old_end)
      p = nextp;
    else
      break;
  }
  assert(nfences >= 2);

  /* Insert the rest of old top into a bin as an ordinary free chunk */
  if (csp != old_top) {
    mchunkptr q = (mchunkptr)old_top;
    size_t psize = csp - old_top;
    mchunkptr tn = chunk_plus_offset(q, psize);
    set_free_with_pinuse(q, psize, tn);
    insert_chunk(m, q, psize);
  }

  check_top_chunk(m, m->top);
}

#if SWEEP_STATS
// atexit() calls malloc, try using __attribute__((destructor)) instead.
__attribute__((destructor))
static void
print_sweep_stats() {
  malloc_printf("Sweeps: %zd.\n", gm->sweepTimes);
  malloc_printf("Swept bytes: %zd.\n", gm->sweptBytes);
  malloc_printf("Frees: %zd.\n", gm->freeTimes);
  malloc_printf("Free bytes: %zd.\n", gm->freeBytes);
  malloc_printf("Bits painted: %zd.\n", gm->bitsPainted);
  malloc_printf("Bits cleared: %zd.\n", gm->bitsCleared);
#if !NO_MALLOC_STATS
  dlmalloc_stats();
#endif
}
#endif // SWEEP_STATS

/* -------------------------- System allocation -------------------------- */

/* Get memory from system using MMAP */
static void* sys_alloc(mstate m, size_t nb) {
  char* tbase = CMFAIL;
  size_t tsize = 0;
  flag_t mmap_flag = 0;
  size_t asize; /* allocation size */

  ensure_initialization();

  /* Directly map large chunks, but only if already initialized */
  if (use_mmap(m) && nb >= mparams.mmap_threshold && m->topsize != 0) {
    void* mem = mmap_alloc(m, nb);
    if (mem != 0)
      return mem;
  }

  asize = granularity_align(nb + SYS_ALLOC_PADDING);
  if (asize <= nb)
    return 0; /* wraparound */
  if (m->footprint_limit != 0) {
    size_t fp = m->footprint + asize;
    if (fp <= m->footprint || fp > m->footprint_limit)
      return 0;
  }

  /*
    Try getting memory in any of three ways (in most-preferred to
    least-preferred order):
    2. A call to MMAP new space (disabled if not HAVE_MMAP).

   In all cases, we need to request enough bytes from system to ensure
   we can malloc nb bytes upon success, so pad with enough space for
   top_foot, plus alignment-pad to make sure we don't lose bytes if
   not on boundary, and round this up to a granularity unit.
  */
  if (HAVE_MMAP && tbase == CMFAIL) {  /* Try MMAP */
    char* mp = (char*)(CALL_MMAP(asize));
#if defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__)
    if(MMAP_SHADOW(mp, asize) != (void*)((size_t)mp>>MALLOC_ALIGN_BITSHIFT)) {
      ABORT;
    }
#endif
    if (mp != CMFAIL) {
      tbase = mp;
      tsize = asize;
      mmap_flag = USE_MMAP_BIT;
    }
  }

////////////////////////
  if (HAVE_MORECORE && tbase == CMFAIL) { /* Try noncontiguous MORECORE */
    if (asize < HALF_MAX_SIZE_T) {
      char* br = CMFAIL;
      char* end = CMFAIL;
      ACQUIRE_MORECORE_LOCK();

      br = (char*)(CALL_MORECORE(asize));
      end = (char*)(CALL_MORECORE(0));
      RELEASE_MORECORE_LOCK();
      if (br != CMFAIL && end != CMFAIL && br < end) {
        size_t ssize = end - br;
        if (ssize > nb + TOP_FOOT_SIZE) {
          tbase = br;
          tsize = ssize;
        }
      }
    }
  }

/////////////////////

  if (tbase != CMFAIL) {

    if ((m->footprint += tsize) > m->max_footprint)
      m->max_footprint = m->footprint;

    if (!is_initialized(m)) { /* first-time initialization */
      if (m->least_addr == 0 || tbase < m->least_addr)
        m->least_addr = tbase;
      m->seg.base = tbase;
      m->seg.size = tsize;
      m->seg.sflags = mmap_flag;
#ifdef CAPREVOKE
      if (do_revocation(m) &&
          caprevoke_shadow(CAPREVOKE_SHADOW_NOVMMAP, tbase, &m->seg.shadow) != 0)
        ABORT;
#endif
      m->magic = mparams.magic;
      m->release_checks = MAX_RELEASE_CHECK_RATE;
      init_bins(m);
      if (is_global(m))
        init_top(m, (mchunkptr)tbase, tsize - TOP_FOOT_SIZE);
      else
      {
        /* Offset top by embedded malloc_state */
        mchunkptr mn = next_chunk(mem2chunk(m));
        init_top(m, mn, (size_t)((tbase + tsize) - (char*)mn) -TOP_FOOT_SIZE);
      }
    }

    else {
#ifndef __CHERI_PURE_CAPABILITY__
      /* Try to merge with an existing segment */
      msegmentptr sp = &m->seg;
      /* Only consider most recent segment if traversal suppressed */
      while (sp != 0 && tbase != sp->base + sp->size)
        sp = (NO_SEGMENT_TRAVERSAL) ? 0 : sp->next;
      if (sp != 0 &&
          !is_extern_segment(sp) &&
          (sp->sflags & USE_MMAP_BIT) == mmap_flag &&
          segment_holds(sp, m->top)) { /* append */
        sp->size += tsize;
        init_top(m, m->top, m->topsize + tsize);
      }
      else {
        if (tbase < m->least_addr)
          m->least_addr = tbase;
        sp = &m->seg;
        while (sp != 0 && sp->base != tbase + tsize)
          sp = (NO_SEGMENT_TRAVERSAL) ? 0 : sp->next;
        if (sp != 0 &&
            !is_extern_segment(sp) &&
            (sp->sflags & USE_MMAP_BIT) == mmap_flag) {
          char* oldbase = sp->base;
          sp->base = tbase;
          sp->size += tsize;
          return prepend_alloc(m, tbase, oldbase, nb);
        }
        else
#endif /* !__CHERI_PURE_CAPABILITY__ */
          add_segment(m, tbase, tsize, mmap_flag);
#ifndef __CHERI_PURE_CAPABILITY__
      }
#endif /* !__CHERI_PURE_CAPABILITY__ */
    }

    if (nb < m->topsize) { /* Allocate from new or extended top space */
      size_t rsize = m->topsize -= nb;
      mchunkptr p = m->top;
      mchunkptr r = m->top = chunk_plus_offset(p, nb);
      r->head = rsize | PINUSE_BIT;
      set_size_and_pinuse_of_inuse_chunk(m, p, nb);
      check_top_chunk(m, m->top);
      check_malloced_chunk(m, chunk2mem(p), nb);
      return chunk2mem(p);
    }
  }

  MALLOC_FAILURE_ACTION;
  return 0;
}

/* -----------------------  system deallocation -------------------------- */

/* Unmap and unlink any mmapped segments that don't contain used chunks */
static size_t release_unused_segments(mstate m) {
  size_t released = 0;
  int nsegs = 0;
  msegmentptr pred = &m->seg;
  msegmentptr sp = pred->next;
  while (sp != 0) {
    char* base = sp->base;
    size_t size = sp->size;
    msegmentptr next = sp->next;
    ++nsegs;
    if (is_mmapped_segment(sp) && !is_extern_segment(sp)) {
      mchunkptr p = align_as_chunk(base);
      size_t psize = chunksize(p);
      /* Can unmap if first chunk holds entire segment and not pinned */
      if (!is_inuse(p) && (char*)p + psize >= base + size - TOP_FOOT_SIZE) {
        tchunkptr tp = (tchunkptr)p;
        assert(segment_holds(sp, (char*)sp));
        if (p == m->dv) {
          m->dv = 0;
          m->dvsize = 0;
        }
        else {
          unlink_large_chunk(m, tp);
        }
        if (CALL_MUNMAP(base, size) == 0) {
#if defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__)
          MUNMAP_SHADOW(base, size);
#endif
          released += size;
          m->footprint -= size;
          /* unlink obsoleted record */
          sp = pred;
          sp->next = next;
        }
        else { /* back out if cannot unmap */
          insert_large_chunk(m, tp, psize);
        }
      }
    }
    if (NO_SEGMENT_TRAVERSAL) /* scan only first segment */
      break;
    pred = sp;
    sp = next;
  }
  /* Reset check counter */
  m->release_checks = (((size_t) nsegs > (size_t) MAX_RELEASE_CHECK_RATE)?
                       (size_t) nsegs : (size_t) MAX_RELEASE_CHECK_RATE);
  return released;
}

static int sys_trim(mstate m, size_t pad) {
  size_t released = 0;
  ensure_initialization();
  if (pad < MAX_REQUEST && is_initialized(m)) {
    pad += TOP_FOOT_SIZE; /* ensure enough room for segment overhead */

    if (m->topsize > pad) {
      /* Shrink top space in granularity-size units, keeping at least one */
      size_t unit = mparams.granularity;
      size_t extra = ((m->topsize - pad + (unit - SIZE_T_ONE)) / unit -
                      SIZE_T_ONE) * unit;
      msegmentptr sp = segment_holding(m, (char*)m->top);

      if (!is_extern_segment(sp)) {
        if (is_mmapped_segment(sp)) {
          if (HAVE_MMAP &&
              sp->size >= extra &&
              !has_segment_link(m, sp)) { /* can't shrink if pinned */
            size_t newsize = sp->size - extra;
            (void)newsize; /* placate people compiling -Wunused-variable */
            /* Prefer mremap, fall back to munmap */
            if ((CALL_MREMAP(sp->base, sp->size, newsize, 0) != MFAIL) ||
                (CALL_MUNMAP(sp->base + newsize, extra) == 0)) {
#if defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__)
              MUNMAP_SHADOW(sp->base+newsize, extra);
#endif
              released = extra;
            }
          }
        }
      }

      if (released != 0) {
        sp->size -= released;
        m->footprint -= released;
        init_top(m, m->top, m->topsize - released);
        check_top_chunk(m, m->top);
      }
    }

    /* Unmap any unused mmapped segments */
    if (HAVE_MMAP)
      released += release_unused_segments(m);

    /* On failure, disable autotrim to avoid repeated failed future calls */
    if (released == 0 && m->topsize > m->trim_check)
      m->trim_check = MAX_SIZE_T;
  }

  return (released != 0)? 1 : 0;
}

/* Consolidate and bin a chunk. Differs from exported versions
   of free mainly in that the chunk need not be marked as inuse.
*/
static void dispose_chunk(mstate m, mchunkptr p, size_t psize) {
  mchunkptr next = chunk_plus_offset(p, psize);
  if (!pinuse(p)) {
    mchunkptr prev;
    size_t prevsize = p->prev_foot;
    if (is_mmapped(p)) {
      psize += prevsize + MMAP_FOOT_PAD;
      if (CALL_MUNMAP((char*)p - prevsize, psize) == 0) {
#if defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__)
        MUNMAP_SHADOW((char*)p - prevsize, psize);
#endif
        m->footprint -= psize;
      }
      return;
    }
    prev = chunk_minus_offset(p, prevsize);
    check_freebuf_corrupt(m, prev);
    psize += prevsize;
    p = prev;
    if (RTCHECK(ok_address(m, prev))) { /* consolidate backward */
      if (p != m->dv) {
        unlink_chunk(m, p, prevsize);
      }
      else if ((next->head & INUSE_BITS) == INUSE_BITS) {
        m->dvsize = psize;
        set_free_with_pinuse(p, psize, next);
        return;
      }
    }
    else {
      CORRUPTION_ERROR_ACTION(m);
      return;
    }
  }
  if (RTCHECK(ok_address(m, next))) {
    // Only consolidate forward if the next is free and not dirty.
    if(!cinuse(next)) {
      check_freebuf_corrupt(m, next);
      if (next == m->top) {
        size_t tsize = m->topsize += psize;
        clear_meta(m->top, m->topsize);
        m->top = p;
        p->head = tsize | PINUSE_BIT;
        if (p == m->dv) {
          m->dv = 0;
          m->dvsize = 0;
        }
        return;
      }
      else if (next == m->dv) {
        size_t dsize = m->dvsize += psize;
        m->dv = p;
        set_size_and_pinuse_of_free_chunk(p, dsize);
        return;
      }
      else {
        size_t nsize = chunksize(next);
        psize += nsize;
        unlink_chunk(m, next, nsize);
        set_size_and_pinuse_of_free_chunk(p, psize);
        if (p == m->dv) {
          m->dvsize = psize;
          return;
        }
      }
    }
    else {
      set_free_with_pinuse(p, psize, next);
    }
    insert_chunk(m, p, psize);
  }
  else {
    CORRUPTION_ERROR_ACTION(m);
  }
}

/* ---------------------------- malloc --------------------------- */

/* allocate a large request from the best fitting chunk in a treebin */
static void* tmalloc_large(mstate m, size_t nb) {
  tchunkptr v = 0;
  size_t rsize = -nb; /* Unsigned negation */
  tchunkptr t;
  bindex_t idx;
  compute_tree_index(nb, idx);
  if ((t = *treebin_at(m, idx)) != 0) {
    /* Traverse tree for this bin looking for node with size == nb */
    size_t sizebits = nb << leftshift_for_tree_index(idx);
    tchunkptr rst = 0;  /* The deepest untaken right subtree */
    for (;;) {
      tchunkptr rt;
      size_t trem = chunksize(t) - nb;
      if (trem < rsize) {
        v = t;
        if ((rsize = trem) == 0)
          break;
      }
      rt = t->child[1];
      t = t->child[(sizebits >> (SIZE_T_BITSIZE-SIZE_T_ONE)) & 1];
      if (rt != 0 && rt != t)
        rst = rt;
      if (t == 0) {
        t = rst; /* set t to least subtree holding sizes > nb */
        break;
      }
      sizebits <<= 1;
    }
  }
  if (t == 0 && v == 0) { /* set t to root of next non-empty treebin */
    binmap_t leftbits = left_bits(idx2bit(idx)) & m->treemap;
    if (leftbits != 0) {
      bindex_t i;
      binmap_t leastbit = least_bit(leftbits);
      compute_bit2idx(leastbit, i);
      t = *treebin_at(m, i);
    }
  }

  while (t != 0) { /* find smallest of tree or subtree */
    size_t trem = chunksize(t) - nb;
    if (trem < rsize) {
      rsize = trem;
      v = t;
    }
    t = leftmost_child(t);
  }

  /*  If dv is a better fit, return 0 so malloc will use it */
  if (v != 0 && rsize < (size_t)(m->dvsize - nb)) {
    if (RTCHECK(ok_address(m, v))) { /* split */
      mchunkptr r = chunk_plus_offset(v, nb);
      assert(chunksize(v) == rsize + nb);
      if (RTCHECK(ok_next(v, r))) {
        unlink_large_chunk(m, v);
        if (rsize < MIN_CHUNK_SIZE)
          set_inuse_and_pinuse(m, v, (rsize + nb));
        else {
          set_size_and_pinuse_of_inuse_chunk(m, v, nb);
          set_size_and_pinuse_of_free_chunk(r, rsize);
          insert_chunk(m, r, rsize);
        }
        return chunk2mem(v);
      }
    }
    CORRUPTION_ERROR_ACTION(m);
  }
  return 0;
}

/* allocate a small request from the best fitting chunk in a treebin */
static void* tmalloc_small(mstate m, size_t nb) {
  tchunkptr t, v;
  size_t rsize;
  bindex_t i;
  binmap_t leastbit = least_bit(m->treemap);
  compute_bit2idx(leastbit, i);
  v = t = *treebin_at(m, i);
  rsize = chunksize(t) - nb;

  while ((t = leftmost_child(t)) != 0) {
    size_t trem = chunksize(t) - nb;
    if (trem < rsize) {
      rsize = trem;
      v = t;
    }
  }

  if (RTCHECK(ok_address(m, v))) {
    mchunkptr r = chunk_plus_offset(v, nb);
    assert(chunksize(v) == rsize + nb);
    if (RTCHECK(ok_next(v, r))) {
      unlink_large_chunk(m, v);
      if (rsize < MIN_CHUNK_SIZE)
        set_inuse_and_pinuse(m, v, (rsize + nb));
      else {
        set_size_and_pinuse_of_inuse_chunk(m, v, nb);
        set_size_and_pinuse_of_free_chunk(r, rsize);
        replace_dv(m, r, rsize);
      }
      return chunk2mem(v);
    }
  }

  CORRUPTION_ERROR_ACTION(m);
  return 0;
}

static void* internal_memalign(mstate m, size_t alignment, size_t bytes);

static void* internal_malloc(mstate m, size_t bytes) {
  /*
     Basic algorithm:
     If a small request (< 256 bytes minus per-chunk overhead):
       1. If one exists, use a remainderless chunk in associated smallbin.
          (Remainderless means that there are too few excess bytes to
          represent as a chunk.)
       2. If it is big enough, use the dv chunk, which is normally the
          chunk adjacent to the one used for the most recent small request.
       3. If one exists, split the smallest available chunk in a bin,
          saving remainder in dv.
       4. If it is big enough, use the top chunk.
       5. If available, get memory from system and use it
     Otherwise, for a large request:
       1. Find the smallest available binned chunk that fits, and use it
          if it is better fitting than dv chunk, splitting if necessary.
       2. If better fitting than any binned chunk, use the dv chunk.
       3. If it is big enough, use the top chunk.
       4. If request size >= mmap threshold, try to directly mmap this chunk.
       5. If available, get memory from system and use it

     The ugly goto's here ensure that postaction occurs along all paths.
  */

#if USE_LOCKS
  ensure_initialization(); /* initialize in sys_alloc if not using locks */
#endif

  if (!PREACTION(m)) {
    void* mem;
    size_t nb;
    if (bytes <= MAX_SMALL_REQUEST) {
      bindex_t idx;
      binmap_t smallbits;
      nb = (bytes < MIN_REQUEST)? MIN_CHUNK_SIZE : pad_request(bytes);
      idx = small_index(nb);
      smallbits = m->smallmap >> idx;

      if ((smallbits & 0x3U) != 0) { /* Remainderless fit to a smallbin. */
        mchunkptr b, p;
        idx += ~smallbits & 1;       /* Uses next bin if idx empty */
        b = smallbin_at(m, idx);
        p = b->fd;
        assert(chunksize(p) == small_index2size(idx));
        unlink_first_small_chunk(m, b, p, idx);
        set_inuse_and_pinuse(m, p, small_index2size(idx));
        mem = chunk2mem(p);
        check_malloced_chunk(m, mem, nb);
        goto postaction;
      }

      else if (nb > m->dvsize) {
        if (smallbits != 0) { /* Use chunk in next nonempty smallbin */
          mchunkptr b, p, r;
          size_t rsize;
          bindex_t i;
          binmap_t leftbits = (smallbits << idx) & left_bits(idx2bit(idx));
          binmap_t leastbit = least_bit(leftbits);
          compute_bit2idx(leastbit, i);
          b = smallbin_at(m, i);
          p = b->fd;
          assert(chunksize(p) == small_index2size(i));
          unlink_first_small_chunk(m, b, p, i);
          rsize = small_index2size(i) - nb;
          /* Fit here cannot be remainderless if 4byte sizes */
          if (SIZE_T_SIZE != 4 && rsize < MIN_CHUNK_SIZE)
            set_inuse_and_pinuse(m, p, small_index2size(i));
          else {
            set_size_and_pinuse_of_inuse_chunk(m, p, nb);
            r = chunk_plus_offset(p, nb);
            set_size_and_pinuse_of_free_chunk(r, rsize);
            replace_dv(m, r, rsize);
          }
          mem = chunk2mem(p);
          check_malloced_chunk(m, mem, nb);
          goto postaction;
        }

        else if (m->treemap != 0 && (mem = tmalloc_small(m, nb)) != 0) {
          check_malloced_chunk(m, mem, nb);
          goto postaction;
        }
      }
    }
    else if (bytes >= MAX_REQUEST)
      nb = MAX_SIZE_T; /* Too big to allocate. Force failure (in sys alloc) */
    else {
      nb = pad_request(bytes);
      if (m->treemap != 0 && (mem = tmalloc_large(m, nb)) != 0) {
        check_malloced_chunk(m, mem, nb);
        goto postaction;
      }
    }

    if (nb <= m->dvsize) {
      size_t rsize = m->dvsize - nb;
      mchunkptr p = m->dv;
      if (rsize >= MIN_CHUNK_SIZE) { /* split dv */
        mchunkptr r = m->dv = chunk_plus_offset(p, nb);
        m->dvsize = rsize;
        set_size_and_pinuse_of_free_chunk(r, rsize);
        set_size_and_pinuse_of_inuse_chunk(m, p, nb);
      }
      else { /* exhaust dv */
        size_t dvs = m->dvsize;
        m->dvsize = 0;
        m->dv = 0;
        set_inuse_and_pinuse(m, p, dvs);
      }
      mem = chunk2mem(p);
      check_malloced_chunk(m, mem, nb);
      goto postaction;
    }

    else if (nb < m->topsize) { /* Split top */
      size_t rsize = m->topsize -= nb;
      mchunkptr p = m->top;
      mchunkptr r = m->top = chunk_plus_offset(p, nb);
      r->head = rsize | PINUSE_BIT;
      set_size_and_pinuse_of_inuse_chunk(m, p, nb);
      mem = chunk2mem(p);
      check_top_chunk(m, m->top);
      check_malloced_chunk(m, mem, nb);
      goto postaction;
    }

    mem = sys_alloc(m, nb);

  postaction:
    ;
    mchunkptr ret = mem2chunk(mem);
    if(!is_mmapped(ret)) {
      size_t theSize = chunksize(ret);
      mchunkptr theNext = chunk_plus_offset(ret, theSize);
      // Clear these two bits to be safe.
      clear_cdirty(ret);
      clear_pdirty(theNext);
    }
    POSTACTION(m);
    UTRACE(0, bytes, mem);
    m->allocated += chunksize(mem2chunk(mem));
    if (m->allocated > m->max_allocated)
      m->max_allocated = m->allocated;
    return mem;
  }

  return 0;
}

static void* dlmalloc_internal_unbounded(size_t bytes) {
  void *mem;
#ifdef __CHERI_PURE_CAPABILITY__
  bytes = __builtin_cheri_round_representable_length(bytes);
  size_t mask = __builtin_cheri_representable_alignment_mask(bytes);
  size_t align = 1 + ~mask;

  if (mask != MAX_SIZE_T && align > MALLOC_ALIGNMENT) {
    mem = internal_memalign(gm, align, bytes);
  } else
#endif
    mem = internal_malloc(gm, bytes);

  assert(chunksize(mem2chunk(mem)) >= bytes + CHUNK_HEADER_OFFSET);

  return mem;
}

void* dlmalloc(size_t bytes) {
    return bound_ptr(dlmalloc_internal_unbounded(bytes), bytes);
}

/* ---------------------------- free --------------------------- */

static void
dlfree_internal(void* mem) {
  /*
     Consolidate freed chunks with preceeding or succeeding bordering
     free chunks, if they exist, and then place in a bin.  Intermixed
     with special cases for top, dv, mmapped chunks, and usage errors.
  */

  if (mem != 0) {
    mchunkptr p  = mem2chunk(mem);
#ifdef __CHERI_PURE_CAPABILITY__
    p->pad = NULL;
#endif
#define fm gm

#if SUPPORT_UNMAP
    char *remap_base = NULL, *remap_end = NULL;
    if (cunmapped(p)) {
      remap_base = __builtin_align_up(chunk2mem(p), mparams.page_size);
      remap_end = __builtin_align_down((char *)p + chunksize(p),
                                       mparams.page_size);
      ptrdiff_t remap_len = remap_end - remap_base;
      assert(remap_len > 0 && remap_len % mparams.page_size == 0);
#ifdef VERBOSE
      malloc_printf("%s: remapping %ti from %#p\n", __func__, remap_len, remap_base);
#endif
      if (mmap(remap_base, remap_len, PROT_READ|PROT_WRITE,
	       MAP_FIXED | MAP_ANON | MAP_CHERI_NOSETBOUNDS, -1, 0) ==
          MAP_FAILED)
        ABORT;
      assert(fm->freebufbytes_unmapped >= remap_len);
      fm->freebufbytes_unmapped -= remap_len;
    }
#endif /* SUPPORT_UNMAP */

#if ZERO_MEMORY
  /*
   * Zero the memory to ensure we don't leak pointers to other parts
   * of the allocation graph to a consumer.
   *
   * XXX: There are optimization opportunities here including:
   *  - MPROT_QUARANTINE doing clearing.
   *  - An efficent, ranged tag clearing instruction.
   */
#if SUPPORT_UNMAP
    if (cunmapped(p)) {
      assert(remap_base && remap_end);
      size_t leading_size = remap_base - (char *)mem;
      // XXXAR: I'm not sure I understand this code correctly but it seems we
      // need to subtract CHUNK_HEADER_OFFSET to avoid zeroing the next chunk
      size_t trailing_size = (char *)mem + chunksize(p) - remap_end - CHUNK_HEADER_OFFSET;
#ifdef VERBOSE
      malloc_printf("%s: cunmapped: zeroing %ti from %#p\n", __func__, leading_size, mem);
      malloc_printf("%s: cunmapped: zeroing %ti from %#p\n", __func__, trailing_size, remap_end);
#endif
      memset(mem, 0, leading_size);
      memset(remap_end, 0, trailing_size);
    } else
#endif /* SUPPORT_UNMAP */
    {
#ifdef VERBOSE
      malloc_printf("%s: zeroing %ti from %#p\n", __func__, chunksize(p) - CHUNK_HEADER_OFFSET, mem);
#endif
      memset(mem, 0, chunksize(p) - CHUNK_HEADER_OFFSET);
    }
#endif /* ZERO_MEMORY */

#if SUPPORT_UNMAP
    if (cunmapped(p)) {
      clear_cunmapped(p);
    }
#endif

    if (1) {
      check_inuse_chunk(fm, p);
      if (RTCHECK(ok_address(fm, p) && ok_inuse(p))) {
        size_t psize = chunksize(p);
        mchunkptr next = chunk_plus_offset(p, psize);
        if (!pinuse(p)) {
          size_t prevsize = p->prev_foot;
          if (is_mmapped(p)) {
            psize += prevsize + MMAP_FOOT_PAD;
            if (CALL_MUNMAP((char*)p - prevsize, psize) == 0) {
#if defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__)
              MUNMAP_SHADOW((char*)p - prevsize, psize);
#endif
              fm->footprint -= psize;
            }
            goto postaction;
          }
          else {
            mchunkptr prev = chunk_minus_offset(p, prevsize);
            check_freebuf_corrupt(fm, prev);
            psize += prevsize;
	    /* Don't leak freebuf pointers */
	    memset(p, 0, CHUNK_HEADER_OFFSET);
            p = prev;
            if (RTCHECK(ok_address(fm, prev))) {
              if (p != fm->dv) {
                unlink_chunk(fm, p, prevsize);
              }
              else if ((next->head & INUSE_BITS) == INUSE_BITS) {
                fm->dvsize = psize;
                set_free_with_pinuse(p, psize, next);
                goto postaction;
              }
            }
            else {
              malloc_printf("fail: (%#p) >= (M)->least_addr(%#p)\n", prev, fm->least_addr);
              goto erroraction;
            }
          }
        }

        if (RTCHECK(ok_next(p, next) && ok_pinuse(next))) {
          // Consolidate forward only with a non-dirty chunk.
          if(!cinuse(next)) {
            check_freebuf_corrupt(fm, next);
            if (next == fm->top) {
              clear_meta(next, fm->topsize);
              size_t tsize = fm->topsize += psize;
              fm->top = p;
              p->head = dirtybits(p) | tsize | PINUSE_BIT;
              if (p == fm->dv) {
                fm->dv = 0;
                fm->dvsize = 0;
              }
              if (should_trim(fm, tsize))
                sys_trim(fm, 0);
              goto postaction;
            }
            else if (next == fm->dv) {
              size_t dsize = fm->dvsize += psize;
              clear_meta(next, fm->dvsize);
              fm->dv = p;
              set_size_and_pinuse_of_free_chunk(p, dsize);
              goto postaction;
            }
            else {
              size_t nsize = chunksize(next);
              psize += nsize;
              unlink_chunk(fm, next, nsize);
              set_size_and_pinuse_of_free_chunk(p, psize);
              if (p == fm->dv) {
                fm->dvsize = psize;
                goto postaction;
              }
            }
          }
          else
            set_free_with_pinuse(p, psize, next);

          if (is_small(psize)) {
            insert_small_chunk(fm, p, psize);
            check_free_chunk(fm, p);
          }
          else {
            tchunkptr tp = (tchunkptr)p;
            insert_large_chunk(fm, tp, psize);
            check_free_chunk(fm, p);
            if (--fm->release_checks == 0)
              release_unused_segments(fm);
          }
          goto postaction;
        }
      }
    erroraction:
      USAGE_ERROR_ACTION(fm, p);
    postaction:
      ;
    }
  }
#undef fm
}

#if defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__)
// The following two functions assume 8 bits in a byte.
static void
shadow_paint(void* start, size_t size) {
  size_t realStart = (size_t)start>>MALLOC_ALIGN_BYTESHIFT;
  size_t realEnd = (((size_t)start+size)>>MALLOC_ALIGN_BYTESHIFT);
  char* firstByte = (char*)(realStart>>3);
  char* lastByte = (char*)(realEnd>>3);
  if(firstByte == lastByte) { // All the paint bits are within a single byte.
    for(size_t bitoff=(realStart&BYTE_ALIGN_MASK); bitoff<(realEnd&BYTE_ALIGN_MASK); bitoff++) {
      *firstByte |= 1<<bitoff;
#if SWEEP_STATS
      gm->bitsPainted++;
#endif
    }
  } else {
    char* nextByte = firstByte+1;
    for(size_t bitoff=(realStart&BYTE_ALIGN_MASK); bitoff<8; bitoff++) {
      *firstByte |= 1<<bitoff;
#if SWEEP_STATS
      gm->bitsPainted++;
#endif
    }
    memset(nextByte, 0xff, lastByte-nextByte);
#if SWEEP_STATS
    gm->bitsPainted += 8*(lastByte-nextByte);
#endif
    for(size_t bitoff=0; bitoff<(realEnd&BYTE_ALIGN_MASK); bitoff++) {
      *lastByte |= 1<<bitoff;
#if SWEEP_STATS
      gm->bitsPainted++;
#endif
    }
  }
}

static void
shadow_clear(void* start, size_t size) {
  size_t realStart = (size_t)start>>MALLOC_ALIGN_BYTESHIFT;
  size_t realEnd = (((size_t)start+size)>>MALLOC_ALIGN_BYTESHIFT);
  char* firstByte = (char*)(realStart>>3);
  char* lastByte = (char*)(realEnd>>3);
  if(firstByte == lastByte) { // All the paint bits are within a single byte.
    for(size_t bitoff=(realStart&BYTE_ALIGN_MASK); bitoff<(realEnd&BYTE_ALIGN_MASK); bitoff++) {
      *firstByte &= ~(1<<bitoff);
#if SWEEP_STATS
      gm->bitsCleared++;
#endif
    }
  } else {
    char* nextByte = firstByte+1;
    for(size_t bitoff=(realStart&BYTE_ALIGN_MASK); bitoff<8; bitoff++) {
      *firstByte &= ~(1<<bitoff);
#if SWEEP_STATS
      gm->bitsCleared++;
#endif
    }
    memset(nextByte, 0, lastByte-nextByte);
#if SWEEP_STATS
    gm->bitsCleared += 8*(lastByte-nextByte);
#endif
    for(size_t bitoff=0; bitoff<(realEnd&BYTE_ALIGN_MASK); bitoff++) {
      *lastByte &= ~(1<<bitoff);
#if SWEEP_STATS
      gm->bitsCleared++;
#endif
    }
  }
}
#endif   /* defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__) */

static void malloc_revoke_internal(const char *cause);

void
dlfree(void* mem) {
    if (mem == 0) return;
#define fm gm

    UTRACE(mem, 0, 0);
    if(!PREACTION(fm)) {
      msegmentptr sp;
      mchunkptr p  = mem2chunk(unbound_ptr(gm, &sp, mem));
      gm->allocated -= chunksize(p);
#if SUPPORT_UNMAP
      void *unmap_base = NULL;
      void *unmap_end = NULL;
#endif

#ifdef __CHERI_PURE_CAPABILITY__
      /*
       * Replace the pointer to the allocation.  This allows us to catch
       * double-free()s in unbound_ptr().  In the CAPREVOKE case, it also
       * allows us to cache the shadow pointer for later use.
       */
#ifdef CAPREVOKE
      p->pad = sp->shadow;
#else
      p->pad = NULL;
#endif
#endif /* __CHERI_PURE_CAPABILITY__ */
      check_inuse_chunk(fm, p);
#if SWEEP_STATS
      fm->freeTimes++;
      fm->freeBytes += chunksize(p);
#endif // SWEEP_STATS
      check_freebuf_corrupt(fm, p);
      if(!do_revocation(fm)) {
        dlfree_internal(chunk2mem(p));
        POSTACTION(fm);
        return;
      }
#if CONSOLIDATE_ON_FREE == 1
      if(!is_mmapped(p)) {
        if(RTCHECK(ok_address(fm, p) && ok_inuse(p))) {
          size_t psize = chunksize(p);
          if(pdirty(p)) {
            size_t prevsize = p->prev_foot;
            mchunkptr prev = chunk_minus_offset(p, prevsize);
            psize += prevsize;
#if SUPPORT_UNMAP
            if (cunmapped(prev)) {
              /*
               * The previous chunk is unmapped up to the page contining our
               * chunk header.  We must unmap that page if we can to
               * join the unmapped region.
               */
              unmap_base = __builtin_align_down(p, mparams.page_size);
            }
#endif
            p = prev;
            if(RTCHECK(ok_address(fm, prev))) {
              unlink_freebuf_chunk(fm, p);
            } else
              CORRUPTION_ERROR_ACTION(fm);
          }

          mchunkptr next = chunk_plus_offset(p, psize);
          if(RTCHECK(ok_next(p, next) && ok_pinuse(next))) {
            // Consolidate forward only with a non-dirty chunk.
            if(cdirty(next)) {
              size_t nsize = chunksize(next);
              psize += nsize;
              unlink_freebuf_chunk(fm, next);
              set_size_and_clear_pdirty_of_dirty_chunk(p, psize);
#if SUPPORT_UNMAP
              if (cunmapped(next)) {
                /*
                 * The page after our allocation is unmapped and we
                 * must join it if we can and consume remainder of the
                 * next chunk.
                 */
                set_cunmapped(p);
                unmap_end = __builtin_align_up(chunk2mem(next), mparams.page_size);
              }
#endif
            }
            else {
              set_size_and_clear_pdirty_of_dirty_chunk(p, psize);
              set_pdirty(next);
            }
          } else
            CORRUPTION_ERROR_ACTION(fm);
        } else {
          USAGE_ERROR_ACTION(fm, p);
		  }
      }
#endif /* CONSOLIDATE_ON_FREE == 1 */
      insert_freebuf_chunk(fm, p);

#if SUPPORT_UNMAP
      if (unmap_base == NULL)
        unmap_base = __builtin_align_up(chunk2mem(p), mparams.page_size);
      if (unmap_end == NULL)
        unmap_end = __builtin_align_down((char *)p + chunksize(p), mparams.page_size);
      ptrdiff_t unmap_len = (char *)unmap_end - (char *)unmap_base;
      if (unmap_len > 0 && (size_t)unmap_len >= mparams.unmap_threshold) {
        set_cunmapped(p);
      }
      /*
       * cunmapped(p) may be set by the threshold check above or be
       * inherited from an ajoining region containing which has previously
       * unmapped.  This means the unmaps smaller than the threshold may
       * occur, but ensures that all of a chunk's potentially
       * unmapable pages are unmapped.
       */
      if (cunmapped(p) && unmap_end > unmap_base) {
#ifdef VERBOSE
        malloc_printf("%s: unmapping %ti from %#p\n", __func__, unmap_len, unmap_base);
#endif
        /*
         * We'd like to unmap the memory, but that could lead to reuse.
         * Instead, map it MAP_GUARD.
         */
#if EMULATE_MADV_REVOKE
        if (madvise(unmap_base, unmap_len, MADV_FREE) != 0)
          CORRUPTION_ERROR_ACTION(fm);
#else
        if (mmap(unmap_base, unmap_len, PROT_NONE,
            MAP_FIXED | MAP_GUARD | MAP_CHERI_NOSETBOUNDS, -1, 0) ==
            MAP_FAILED)
          CORRUPTION_ERROR_ACTION(fm);
#endif
        fm->freebufbytes_unmapped += unmap_len;
        assert(fm->freebufbytes_unmapped <= fm->freebufbytes);
      }
#endif

      size_t freebufbytes_mapped = fm->freebufbytes - fm->freebufbytes_unmapped;
      /* fm->freebufbytes is included in fm->footprint, so adjust it there as well */
      size_t footprint_mapped = fm->footprint - fm->freebufbytes_unmapped;
      if (freebufbytes_mapped > mparams.max_freebufbytes) {
        malloc_revoke_internal("mparams.max_freebufbytes exceeded");
      } else if (freebufbytes_mapped > mparams.min_freebufbytes &&
                 freebufbytes_mapped >
                 (size_t)(footprint_mapped * mparams.max_freebuf_percent)) {
        malloc_revoke_internal("mparams.max_freebuf_percent and "
	                       "mparams.min_freebufbytes exceeded");
      }
      POSTACTION(fm);
    }
}

static void
malloc_revoke_internal(const char *reason) {
#ifdef VERBOSE
  malloc_printf("%s: %s\n", __func__, reason);
#endif
#if SWEEP_STATS
  gm->sweepTimes++;
  gm->sweptBytes += gm->footprint;
#endif // SWEEP_STATS
  mchunkptr freebin = &gm->freebufbin;
  for (mchunkptr thePtr = freebin->fd; thePtr != freebin; thePtr = thePtr->fd) {
#if defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__)
    shadow_paint(thePtr, chunksize(thePtr));
#else
#ifdef CAPREVOKE
    vaddr_t addr = __builtin_cheri_address_get(chunk2mem(thePtr));
    caprev_shadow_nomap_set_raw(thePtr->pad, addr, addr + chunksize(thePtr));
#endif
#endif   /* defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__) */
  }

#ifdef CAPREVOKE
  struct caprevoke_stats crst;
  if (cri == NULL) {
    int error;
    error = caprevoke_shadow(CAPREVOKE_SHADOW_INFO_STRUCT, NULL,
        __DECONST(void **, &cri));
    assert(error == 0);
  }

  atomic_thread_fence(memory_order_acq_rel);
  caprevoke_epoch start_epoch = cri->epoch_enqueue;

  while (!caprevoke_epoch_clears(cri->epoch_dequeue, start_epoch)) {
    caprevoke(CAPREVOKE_LAST_PASS, start_epoch, &crst);
  }
#endif

  for (mchunkptr thePtr = freebin->fd; thePtr != freebin; thePtr = freebin->fd) {
    unlink_first_freebuf_chunk(gm, freebin, thePtr);
#if CONSOLIDATE_ON_FREE == 1 || !defined(__CHERI_PURE_CAPABILITY__)
    size_t theSize = chunksize(thePtr);
#endif
#if defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__)
    shadow_clear(thePtr, theSize);
#else
#ifdef CAPREVOKE
    vaddr_t addr = __builtin_cheri_address_get(chunk2mem(thePtr));
    caprev_shadow_nomap_clear_raw(thePtr->pad, addr, addr + chunksize(thePtr));
#endif
#endif   /* defined(REVOKE) && !defined(__CHERI_PURE_CAPABILITY__) */
#if CONSOLIDATE_ON_FREE == 1
    mchunkptr theNext = chunk_plus_offset(thePtr, theSize);
    if(!is_mmapped(thePtr)) {
      assert(cdirty(thePtr));
      assert(pdirty(theNext));
      clear_cdirty(thePtr);
      clear_pdirty(theNext);
    }
#endif /* CONSOLIDATE_ON_FREE == 1 */

    // Have to do free_internal after unlinking, otherwise the circular
    // list of freebufbin will get corrupted.
    dlfree_internal(chunk2mem(thePtr));
  }
#ifdef SUPPORT_UNMAP
  assert(fm->freebufbytes_unmapped == 0);
#endif /* SUPPORT_UNMAP */
}

void
dlmalloc_revoke(void) {
  PREACTION(gm);
  malloc_revoke_internal(__func__);
  POSTACTION(gm);
}

void* dlcalloc(size_t n_elements, size_t elem_size) {
  void* mem;
  size_t req = 0;
  if (n_elements != 0) {
    req = n_elements * elem_size;
    if (((n_elements | elem_size) & ~(size_t)0xffff) &&
        (req / n_elements != elem_size))
      req = MAX_SIZE_T; /* force downstream failure on overflow */
  }
  mem = dlmalloc_internal_unbounded(req);
#if !ZERO_MEMORY
  if (mem != 0 && calloc_must_clear(mem2chunk(mem)))
    memset(mem, 0, req);
#endif
  return bound_ptr(mem, req);
}

/* ------------ Internal support for realloc, memalign, etc -------------- */

/* Try to realloc; only in-place unless can_move true */
static mchunkptr try_realloc_chunk(mstate m, mchunkptr p, size_t nb) {
  mchunkptr newp = 0;
  size_t oldsize = chunksize(p);
  mchunkptr next = chunk_plus_offset(p, oldsize);
  if (RTCHECK(ok_address(m, p) && ok_inuse(p) &&
              ok_next(p, next) && ok_pinuse(next))) {
    if (is_mmapped(p)) {
      newp = mmap_resize(m, p, nb);
    }
    else if (oldsize >= nb) {             /* already big enough */
#if 0
      /*
       * If already big enough, we do not shrink.
       * We CANNOT shrink, because this chunk might have already been used to
       * create a chunk that spans across below and above the new upper bound,
       * which is not a subset of either the remainder or the shrinked chunk.
       */
      size_t rsize = oldsize - nb;
      if (rsize >= MIN_CHUNK_SIZE) {      // Split off remainder.
        mchunkptr r = chunk_plus_offset(p, nb);
        set_inuse(m, p, nb);
        set_inuse(m, r, rsize);
        dispose_chunk(m, r, rsize);
      }
#else
      newp = p;
#endif
    }
    else if (next == m->top) {  /* extend into top */
      assert(!cdirty(next));
      if (oldsize + m->topsize > nb) {
        size_t newsize = oldsize + m->topsize;
        size_t newtopsize = newsize - nb;
        clear_meta(next, m->topsize);
        mchunkptr newtop = chunk_plus_offset(p, nb);
        set_inuse(m, p, nb);
        newtop->head = newtopsize |PINUSE_BIT;
        m->top = newtop;
        m->topsize = newtopsize;
        newp = p;
      }
    }
    else if (next == m->dv) { /* extend into dv */
      size_t dvs = m->dvsize;
      if (oldsize + dvs >= nb) {
        size_t dsize = oldsize + dvs - nb;
        clear_meta(next, dvs);
        if (dsize >= MIN_CHUNK_SIZE) {
          mchunkptr r = chunk_plus_offset(p, nb);
          mchunkptr n = chunk_plus_offset(r, dsize);
          set_inuse(m, p, nb);
          set_size_and_pinuse_of_free_chunk(r, dsize);
          clear_pinuse(n);
          m->dvsize = dsize;
          m->dv = r;
        }
        else { /* exhaust dv */
          size_t newsize = oldsize + dvs;
          set_inuse(m, p, newsize);
          m->dvsize = 0;
          m->dv = 0;
        }
        newp = p;
      }
    }
    else if(!cinuse(next)) {
      assert(!cdirty(next));
      size_t nextsize = chunksize(next);
      if (oldsize + nextsize >= nb) {
        size_t rsize = oldsize + nextsize - nb;
        unlink_chunk(m, next, nextsize);
        if (rsize < MIN_CHUNK_SIZE) {
          size_t newsize = oldsize + nextsize;
          set_inuse(m, p, newsize);
        }
        else {
          mchunkptr r = chunk_plus_offset(p, nb);
          set_inuse(m, p, nb);
          set_inuse(m, r, rsize);
          dispose_chunk(m, r, rsize);
        }
        newp = p;
      }
    }
  }
  else {
    USAGE_ERROR_ACTION(m, chunk2mem(p));
  }
  return newp;
}

static void* internal_memalign(mstate m, size_t alignment, size_t bytes) {
  void* mem = 0;
  if (alignment <  MIN_CHUNK_SIZE) /* must be at least a minimum chunk size */
    alignment = MIN_CHUNK_SIZE;
  if ((alignment & (alignment-SIZE_T_ONE)) != 0) {/* Ensure a power of 2 */
    size_t a = MALLOC_ALIGNMENT << 1;
    while (a < alignment) a <<= 1;
    alignment = a;
  }
  if (bytes >= MAX_REQUEST - alignment) {
    if (m != 0)  { /* Test isn't needed but avoids compiler warning */
      MALLOC_FAILURE_ACTION;
    }
  }
  else {
    size_t nb = request2size(bytes);
    size_t req = nb + alignment + MIN_CHUNK_SIZE - CHUNK_OVERHEAD;
    mem = internal_malloc(m, req);
    if (mem != 0) {
      mchunkptr p = mem2chunk(mem);
      if (PREACTION(m))
        return 0;
      if (!__builtin_is_aligned(mem, alignment)) { /* misaligned */
        /*
          Find an aligned spot inside chunk.  Since we need to give
          back leading space in a chunk of at least MIN_CHUNK_SIZE, if
          the first calculation places us at a spot with less than
          MIN_CHUNK_SIZE leader, we can move to the next aligned spot.
          We've allocated enough total room so that this is always
          possible.
        */
        char* br = (char*)mem2chunk(__builtin_align_up(mem, alignment));
        char* pos = ((size_t)(br - (char*)(p)) >= MIN_CHUNK_SIZE)?
          br : br+alignment;
        mchunkptr newp = (mchunkptr)pos;
        size_t leadsize = pos - (char*)(p);
        size_t newsize = chunksize(p) - leadsize;

        if (is_mmapped(p)) { /* For mmapped chunks, just adjust offset */
          newp->prev_foot = p->prev_foot + leadsize;
          newp->head = newsize;
        }
        else { /* Otherwise, give back leader, use the rest */
          set_inuse(m, newp, newsize);
          set_inuse(m, p, leadsize);
          dispose_chunk(m, p, leadsize);
        }
        p = newp;
      }

      /* Give back spare room at the end */
      if (!is_mmapped(p)) {
        size_t size = chunksize(p);
        if (size > nb + MIN_CHUNK_SIZE) {
          size_t remainder_size = size - nb;
          mchunkptr remainder = chunk_plus_offset(p, nb);
          set_inuse(m, p, nb);
          set_inuse(m, remainder, remainder_size);
          dispose_chunk(m, remainder, remainder_size);
        }
      }

      mem = chunk2mem(p);
      assert (chunksize(p) >= nb);
      assert(__builtin_is_aligned(mem, alignment));
      check_inuse_chunk(m, p);
      POSTACTION(m);
    }
  }
  m->allocated += chunksize(mem2chunk(mem));
  if (m->allocated > m->max_allocated)
    m->max_allocated = m->allocated;
  return mem;
}

/* Traversal */
#if MALLOC_INSPECT_ALL
static void internal_inspect_all(mstate m,
                                 void(*handler)(void *start,
                                                void *end,
                                                size_t used_bytes,
                                                void* callback_arg),
                                 void* arg) {
  if (is_initialized(m)) {
    mchunkptr top = m->top;
    msegmentptr s;
    for (s = &m->seg; s != 0; s = s->next) {
      mchunkptr q = align_as_chunk(s->base);
      while (segment_holds(s, q) && q->head != FENCEPOST_HEAD) {
        mchunkptr next = next_chunk(q);
        size_t sz = chunksize(q);
        size_t used;
        void* start;
        if (is_inuse(q)) {
          used = sz - CHUNK_OVERHEAD; /* must not be mmapped */
          start = chunk2mem(q);
        }
        else {
          used = 0;
          if (is_small(sz)) {     /* offset by possible bookkeeping */
            start = (void*)((char*)q + sizeof(struct malloc_chunk));
          }
          else {
            start = (void*)((char*)q + sizeof(struct malloc_tree_chunk));
          }
        }
        if (start < (void*)next)  /* skip if all space is bookkeeping */
          handler(start, next, used, arg);
        if (q == top)
          break;
        q = next;
      }
    }
  }
}
#endif /* MALLOC_INSPECT_ALL */

/* ------------------ Exported realloc, memalign, etc -------------------- */

void* dlrealloc(void* oldmem, size_t bytes) {
  void* mem = 0;
#ifdef __CHERI_PURE_CAPABILITY__
  bytes = __builtin_cheri_round_representable_length(bytes);
  size_t mask = __builtin_cheri_representable_alignment_mask(bytes);
#endif
  if (oldmem == 0) {
    return dlmalloc(bytes);
  }
  else if (bytes >= MAX_REQUEST) {
    MALLOC_FAILURE_ACTION;
  }
#ifdef REALLOC_ZERO_BYTES_FREES
  else if (bytes == 0) {
    dlfree(oldmem);
    return 0;
  }
#endif /* REALLOC_ZERO_BYTES_FREES */
  else {
    size_t nb = request2size(bytes);
    mchunkptr oldp = mem2chunk(unbound_ptr(gm, NULL, oldmem));
    mstate m = gm;
#ifdef MALLOC_UTRACE
    malloc_utrace_suspend++;
#endif
    if (!PREACTION(m)) {
      mchunkptr newp = 0;
#ifdef __CHERI_PURE_CAPABILITY__
      /*
       * Don't try to expand in place if the extended pointer won't be
       * sufficently aligned.
       */
      if ((__builtin_cheri_base_get(oldmem) & ~mask) == 0)
#endif
        newp = try_realloc_chunk(m, oldp, nb);
      POSTACTION(m);
      if (newp != 0) {
        check_inuse_chunk(m, newp);
        mem = chunk2mem(newp);
        if(!is_mmapped(newp))
          clear_pdirty(next_chunk(newp));
      }
      else {
#ifdef __CHERI_PURE_CAPABILITY__
        size_t align = 1 + ~mask;

        if (mask != MAX_SIZE_T && align > MALLOC_ALIGNMENT) {
          size_t align = 1 + ~mask;
          mem = internal_memalign(gm, align, bytes);
        } else
#endif
          mem = internal_malloc(m, bytes);
        if (mem != 0) {
          size_t oc = chunksize(oldp) - overhead_for(oldp);
	  /*
	   * CHERI: Use chunk2mem(oldp) as the source because oc might be
	   * longer than the length of oldmem.
	   */
          memcpy(mem, chunk2mem(oldp), (oc < bytes)? oc : bytes);
          internal_free(m, oldmem);
        }
      }
    }
#ifdef MALLOC_UTRACE
    malloc_utrace_suspend--;
#endif
    UTRACE(oldmem, bytes, mem);
  }
  return bound_ptr(mem, bytes);
}

int dlposix_memalign(void** pp, size_t alignment, size_t bytes) {
  void* mem = 0;
#ifdef __CHERI_PURE_CAPABILITY__
  bytes = __builtin_cheri_round_representable_length(bytes);
#endif
  if (alignment == MALLOC_ALIGNMENT)
    mem = dlmalloc_internal_unbounded(bytes);
  else {
    size_t d = alignment / sizeof(void*);
    size_t r = alignment % sizeof(void*);
    if (r != 0 || d == 0 || (d & (d-SIZE_T_ONE)) != 0)
      return EINVAL;
    else if (bytes <= MAX_REQUEST - alignment) {
      if (alignment <  MIN_CHUNK_SIZE)
        alignment = MIN_CHUNK_SIZE;
      mem = internal_memalign(gm, alignment, bytes);
    }
  }
  if (mem == 0)
    return ENOMEM;
  else {
    *pp = bound_ptr(mem, bytes);
    return 0;
  }
}

void *
dlaligned_alloc(size_t alignment, size_t size) {
  int error;
  void *mem;

  error = dlposix_memalign(&mem, alignment, size);
  if (error != 0) {
	  printf("error = %d\n", error); while(1);
//    errno = error;
    return 0;
  }
  return mem;
}

#if MALLOC_INSPECT_ALL
void dlmalloc_inspect_all(void(*handler)(void *start,
                                         void *end,
                                         size_t used_bytes,
                                         void* callback_arg),
                          void* arg) {
  ensure_initialization();
  if (!PREACTION(gm)) {
    internal_inspect_all(gm, handler, arg);
    POSTACTION(gm);
  }
}
#endif /* MALLOC_INSPECT_ALL */

int dlmalloc_trim(size_t pad) {
  int result = 0;
  ensure_initialization();
  if (!PREACTION(gm)) {
    result = sys_trim(gm, pad);
    POSTACTION(gm);
  }
  return result;
}

size_t dlmalloc_footprint(void) {
  return gm->footprint;
}

size_t dlmalloc_max_footprint(void) {
  return gm->max_footprint;
}

size_t dlmalloc_footprint_limit(void) {
  size_t maf = gm->footprint_limit;
  return maf == 0 ? MAX_SIZE_T : maf;
}

size_t dlmalloc_set_footprint_limit(size_t bytes) {
  size_t result;  /* invert sense of 0 */
  if (bytes == 0)
    result = granularity_align(1); /* Use minimal size */
  if (bytes == MAX_SIZE_T)
    result = 0;                    /* disable */
  else
    result = granularity_align(bytes);
  return gm->footprint_limit = result;
}

#if !NO_MALLINFO
struct mallinfo dlmallinfo(void) {
  return internal_mallinfo(gm);
}
#endif /* NO_MALLINFO */

#if !NO_MALLOC_STATS
void dlmalloc_stats(void) {
  internal_malloc_stats(gm);
}
#endif /* NO_MALLOC_STATS */

int dlmallopt(int param_number, int value) {
  return change_mparam(param_number, value);
}

__attribute__((always_inline))
void *dlmalloc_underlying_allocation(void *mem) {
  /*
   * unbound_ptr only succeeds if mem points to the beginning of a legitimate
   * allocation (because a consumer can't forge a "pad" capability with VMMAP
   * permissions).
   */
  if (mem != NULL) {
    void *ptr = unbound_ptr(gm, NULL, mem);
    mchunkptr chunk = mem2chunk(ptr);
    if (is_inuse(chunk)) {
      /* bound the pointer without remove VMMAP permission */
      void *bounded = ptr;
#ifdef CHERI_SET_BOUNDS
      bounded = __builtin_cheri_bounds_set(bounded, chunksize(chunk) - overhead_for(chunk));
#endif
//      bounded = __builtin_cheri_perms_and(bounded, CHERI_PERMS_USERSPACE_DATA);
      chunk->pad = ptr;
      return bounded;
      /*return bound_ptr(ptr, chunksize(chunk) - overhead_for(chunk));*/
    }
  }
  return NULL;
}

size_t dlmalloc_usable_size(void* mem) {
  size_t allocation_size = 0;
  if (mem != NULL) {
    mchunkptr p = mem2chunk(unbound_ptr(gm, NULL, mem));
    if (is_inuse(p))
      allocation_size = chunksize(p) - overhead_for(p);
  }

#ifndef __CHERI_PURE_CAPABILITY__
  return allocation_size;
#else
  size_t cap_length = __builtin_cheri_length_get(mem);
  return cap_length < allocation_size ? cap_length : allocation_size;
#endif
}
