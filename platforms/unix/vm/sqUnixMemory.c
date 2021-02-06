/* sqUnixMemory.c -- dynamic memory management
 * 
 * Author: Ian.Piumarta@squeakland.org
 * 
 *   Copyright (C) 1996-2005 by Ian Piumarta and other authors/contributors
 *                              listed elsewhere in this file.
 *   All rights reserved.
 *   
 *   This file is part of Unix Squeak.
 * 
 *   Permission is hereby granted, free of charge, to any person obtaining a
 *   copy of this software and associated documentation files (the "Software"),
 *   to deal in the Software without restriction, including without limitation
 *   the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *   and/or sell copies of the Software, and to permit persons to whom the
 *   Software is furnished to do so, subject to the following conditions:
 * 
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * 
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 */
#include "sq.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_MMAP
#  include <sys/mman.h>
#endif
#include <errno.h>

#include "sqMemoryAccess.h"
#include "debug.h"

#if !SPURVM /* Spur uses sqUnixSpurMemory.c */
void *uxAllocateMemory(usqInt minHeapSize, usqInt desiredHeapSize);

/* Note: 
 * 
 *   The code allows memory to be overallocated; i.e., the initial
 *   block is reserved via mmap() and then the unused portion
 *   munmap()ped from the top end.  This is INHERENTLY DANGEROUS since
 *   malloc() may randomly map new memory in the block we "reserved"
 *   and subsequently unmap()ped.  Enabling this causes crashes in
 *   Croquet, which makes heavy use of the FFI and thus calls malloc()
 *   all over the place.
 *   
 *   For this reason, overallocateMemory is DISABLED by default.
 *   
 *   The upshot of all this is that Squeak will claim (and hold on to)
 *   ALL of the available virtual memory (or at least 75% of it) when
 *   it starts up.  If you can't live with that, use the -memory
 *   option to allocate a fixed size heap.
 */

char *uxGrowMemoryBy(char *oldLimit, sqInt delta);
char *uxShrinkMemoryBy(char *oldLimit, sqInt delta);
sqInt uxMemoryExtraBytesLeft(sqInt includingSwap);

static int	    pageSize = 0;
static unsigned int pageMask = 0;
int mmapErrno = 0;

#if defined(HAVE_MMAP)

#include <fcntl.h>

#if !defined(MAP_ANON)
# if defined(MAP_ANONYMOUS)
#   define MAP_ANON MAP_ANONYMOUS
# else
#   define MAP_ANON 0
# endif
#endif

#define MAP_PROT	(PROT_READ | PROT_WRITE)
#define MAP_FLAGS	(MAP_ANON | MAP_PRIVATE)

extern int useMmap;
/* Since Cog needs to make memory executable via mprotect, and since mprotect
 * only works on mmapped memory we must always use mmap in Cog.
 */
#if COGVM
# define ALWAYS_USE_MMAP 1
#endif

#if SQ_IMAGE32 && SQ_HOST64
char *sqMemoryBase= (char *)-1;
#endif

/*xxx THESE SHOULD BE COMMAND-LINE/ENVIRONMENT OPTIONS */
int overallocateMemory	= 0;	/* see notes above */

static int   devZero	= -1;
static char *heap	=  0;
static int   heapSize	=  0;
static int   heapLimit	=  0;

#define valign(x)	((x) & pageMask)

static int min(int x, int y) { return (x < y) ? x : y; }
static int max(int x, int y) { return (x > y) ? x : y; }


/* answer the address of (minHeapSize <= N <= desiredHeapSize) bytes of memory. */

#if SPURVM
void *
uxAllocateMemory(usqInt minHeapSize, usqInt desiredHeapSize)
{
	if (heap) {
		fprintf(stderr, "uxAllocateMemory: already called\n");
		exit(1);
	}
	pageSize= getpagesize();
	pageMask= ~(pageSize - 1);
#else /* SPURVM */
void *uxAllocateMemory(usqInt minHeapSize, usqInt desiredHeapSize)
{
# if !ALWAYS_USE_MMAP
  if (!useMmap)
    return malloc(desiredHeapSize);
# endif

  if (heap) {
      fprintf(stderr, "uxAllocateMemory: already called\n");
      exit(1);
  }
  pageSize= getpagesize();
  pageMask= ~(pageSize - 1);

  DPRINTF(("uxAllocateMemory: pageSize 0x%x (%d), mask 0x%x\n", pageSize, pageSize, pageMask));

# if (!MAP_ANON)
  if ((devZero= open("/dev/zero", O_RDWR)) < 0) {
      perror("uxAllocateMemory: /dev/zero");
      return 0;
  }
# endif

  DPRINTF(("uxAllocateMemory: /dev/zero descriptor %d\n", devZero));
  DPRINTF(("uxAllocateMemory: min heap %d, desired %d\n", minHeapSize, desiredHeapSize));

  heapLimit= valign(max(desiredHeapSize, useMmap));

  while ((!heap) && (heapLimit >= minHeapSize)) {
      DPRINTF(("uxAllocateMemory: mapping 0x%08x bytes (%d Mbytes)\n", heapLimit, heapLimit >> 20));
      if (MAP_FAILED == (heap= mmap(0, heapLimit, MAP_PROT, MAP_FLAGS, devZero, 0))) {
	  heap= 0;
	  heapLimit= valign(heapLimit / 4 * 3);
	}
  }

  if (!heap) {
      fprintf(stderr, "uxAllocateMemory: failed to allocate at least %lld bytes)\n", (long long)minHeapSize);
      useMmap= 0;
      return malloc(desiredHeapSize);
  }

  heapSize= heapLimit;

  if (overallocateMemory)
    uxShrinkMemoryBy(heap + heapLimit, heapLimit - desiredHeapSize);

  return heap;
}
#endif /* SPURVM */


static int log_mem_delta = 0;
#define MDPRINTF(foo) if (log_mem_delta) DPRINTF(foo); else 0

/* grow the heap by delta bytes.  answer the new end of memory. */

char *uxGrowMemoryBy(char *oldLimit, sqInt delta)
{
  if (useMmap)
    {
      int newSize=  min(valign(oldLimit - heap + delta), heapLimit);
      int newDelta= newSize - heapSize;
      MDPRINTF(("uxGrowMemory: %p By: %d(%d) (%d -> %d)\n", oldLimit, newDelta, delta, heapSize, newSize));
      assert(0 == (newDelta & ~pageMask));
      assert(0 == (newSize  & ~pageMask));
      assert(newDelta >= 0);
      if (newDelta)
	{
	  MDPRINTF(("was: %p %p %p = 0x%x (%d) bytes\n", heap, heap + heapSize, heap + heapLimit, heapSize, heapSize));
	  if (overallocateMemory)
	    {
	      char *base= heap + heapSize;
	      MDPRINTF(("remap: %p + 0x%x (%d)\n", base, newDelta, newDelta));
	      if (MAP_FAILED == mmap(base, newDelta, MAP_PROT, MAP_FLAGS | MAP_FIXED, devZero, heapSize))
		{
		  perror("mmap");
		  return oldLimit;
		}
	    }
	  heapSize += newDelta;
	  MDPRINTF(("now: %p %p %p = 0x%x (%d) bytes\n", heap, heap + heapSize, heap + heapLimit, heapSize, heapSize));
	  assert(0 == (heapSize  & ~pageMask));
	}
      return heap + heapSize;
    }
  return oldLimit;
}


/* shrink the heap by delta bytes.  answer the new end of memory. */

char *uxShrinkMemoryBy(char *oldLimit, sqInt delta)
{
  if (useMmap)
    {
      int newSize=  max(0, valign((char *)oldLimit - heap - delta));
      int newDelta= heapSize - newSize;
      MDPRINTF(("uxGrowMemory: %p By: %d(%d) (%d -> %d)\n", oldLimit, newDelta, delta, heapSize, newSize));
      assert(0 == (newDelta & ~pageMask));
      assert(0 == (newSize  & ~pageMask));
      assert(newDelta >= 0);
      if (newDelta)
	{
	  MDPRINTF(("was: %p %p %p = 0x%x (%d) bytes\n", heap, heap + heapSize, heap + heapLimit, heapSize, heapSize));
	  if (overallocateMemory)
	    {
	      char *base= heap + heapSize - newDelta;
	      MDPRINTF(("unmap: %p + 0x%x (%d)\n", base, newDelta, newDelta));
	      if (munmap(base, newDelta) < 0)
		{
		  perror("unmap");
		  return oldLimit;
		}
	    }
	  heapSize -= newDelta;
	  MDPRINTF(("now: %p %p %p = 0x%x (%d) bytes\n", heap, heap + heapSize, heap + heapLimit, heapSize, heapSize));
	  assert(0 == (heapSize  & ~pageMask));
	}
      return heap + heapSize;
    }
  return oldLimit;
}


/* answer the number of bytes available for growing the heap. */

sqInt uxMemoryExtraBytesLeft(sqInt includingSwap)
{
  return useMmap ? (heapLimit - heapSize) : 0;
}


#else  /* HAVE_MMAP */

# if COG
void *
uxAllocateMemory(sqInt minHeapSize, sqInt desiredHeapSize)
{
	if (pageMask) {
		fprintf(stderr, "uxAllocateMemory: already called\n");
		exit(1);
	}
	pageSize = getpagesize();
	pageMask = ~(pageSize - 1);
#	if SPURVM
	return malloc(desiredHeapSize);
#	else
	return malloc(desiredHeapSize);
#	endif
}
# else /* COG */
void *uxAllocateMemory(sqInt minHeapSize, sqInt desiredHeapSize)	{ return malloc(desiredHeapSize); }
# endif /* COG */
char *uxGrowMemoryBy(char * oldLimit, sqInt delta)			{ return oldLimit; }
char *uxShrinkMemoryBy(char *oldLimit, sqInt delta)			{ return oldLimit; }
sqInt uxMemoryExtraBytesLeft(sqInt includingSwap)			{ return 0; }

#endif /* HAVE_MMAP */



#if SQ_IMAGE32 && SQ_HOST64

usqInt sqAllocateMemory(usqInt minHeapSize, usqInt desiredHeapSize)
{
  sqMemoryBase= uxAllocateMemory(minHeapSize, desiredHeapSize);
  if (!sqMemoryBase) return 0;
  sqMemoryBase -= SQ_FAKE_MEMORY_OFFSET;
  return (sqInt)SQ_FAKE_MEMORY_OFFSET;
}

sqInt sqGrowMemoryBy(sqInt oldLimit, sqInt delta)
{
  return oopForPointer(uxGrowMemoryBy(pointerForOop(oldLimit), delta));
}

sqInt sqShrinkMemoryBy(sqInt oldLimit, sqInt delta)
{
  return oopForPointer(uxShrinkMemoryBy(pointerForOop(oldLimit), delta));
}

sqInt sqMemoryExtraBytesLeft(sqInt includingSwap)
{
  return uxMemoryExtraBytesLeft(includingSwap);
}

#else

usqInt sqAllocateMemory(usqInt minHeapSize, usqInt desiredHeapSize)	{ return (sqInt)(long)uxAllocateMemory(minHeapSize, desiredHeapSize); }
sqInt sqGrowMemoryBy(sqInt oldLimit, sqInt delta)			{ return (sqInt)(long)uxGrowMemoryBy((char *)(long)oldLimit, delta); }
sqInt sqShrinkMemoryBy(sqInt oldLimit, sqInt delta)			{ return (sqInt)(long)uxShrinkMemoryBy((char *)(long)oldLimit, delta); }
sqInt sqMemoryExtraBytesLeft(sqInt includingSwap)			{ return uxMemoryExtraBytesLeft(includingSwap); }

#endif

#define roundDownToPage(v) ((v)&pageMask)
#define roundUpToPage(v) (((v)+pageSize-1)&pageMask)

#if COGVM
#   if DUAL_MAPPED_CODE_ZONE
/* We are indebted to Chris Wellons who designed this elegant API for dual
 * mapping which we depend on for fine-grained code modification (classical
 * Deutsch/Schiffman style inline cacheing and derivatives).  Chris's code is:
	https://nullprogram.com/blog/2016/04/10/
 *
 * To cope with modern OSs that disallow executing code in writable memory we
 * dual-map the code zone, one mapping with read/write permissions and the other
 * with read/execute permissions. In such a configuration the code zone has
 * already been alloated and is not included in (what is no longer) the initial
 * alloc.
 */
static void
memory_alias_map(size_t size, size_t naddr, void **addrs)
{
extern char  *exeName;
	char path[128];
	snprintf(path, sizeof(path), "/%s(%lu,%p)",
			 exeName ? exeName : __FUNCTION__, (long)getpid(), addrs);
	int fd = shm_open(path, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (fd == -1) {
		perror("memory_alias_map: shm_open");
		exit(0666);
	}
	shm_unlink(path);
	ftruncate(fd, size);
	for (size_t i = 0; i < naddr; i++) {
		addrs[i] = mmap(addrs[i], size, PROT_READ | PROT_WRITE,
								addrs[i] ? MAP_FIXED | MAP_SHARED : MAP_SHARED,
								fd, 0);
		if (addrs[i] == MAP_FAILED) {
			perror("memory_alias_map: mmap(addrs[i]...");
			exit(0667);
		}
	}
	close(fd);
	return;
}
#   endif /* DUAL_MAPPED_CODE_ZONE */

void
sqMakeMemoryExecutableFromToCodeToDataDelta(usqInt startAddr,
											usqInt endAddr,
											sqInt *codeToDataDelta)
{
	usqInt firstPage = roundDownToPage(startAddr);
	usqInt size = endAddr - firstPage;

#  if DUAL_MAPPED_CODE_ZONE
	usqInt mappings[2];

	mappings[0] = firstPage;
	mappings[1] = 0;

	memory_alias_map(size, 2, (void **)mappings);
	assert(mappings[0] == firstPage);
	*codeToDataDelta = mappings[1] - startAddr;

	if (mprotect((void *)firstPage,
				 size,
				 PROT_READ | PROT_EXEC) < 0)
		perror("mprotect(x,y,PROT_READ | PROT_EXEC)");

#  else /* DUAL_MAPPED_CODE_ZONE */

	if (mprotect((void *)firstPage,
				 size,
				 PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
		perror("mprotect(x,y,PROT_READ | PROT_WRITE | PROT_EXEC)");

	assert(!codeToDataDelta);
#  endif
}

void
sqMakeMemoryNotExecutableFromTo(usqInt startAddr, usqInt endAddr)
{
# if 0
	usqInt firstPage = roundDownToPage(startAddr);
	/* Arguably this is pointless since allocated memory always does include
	 * write permission.  Annoyingly the mprotect call fails on both linux &
	 * mac os x.  So make the whole thing a nop.
	 */
	if (mprotect((void *)firstPage,
				 endAddr - firstPage + 1,
				 PROT_READ | PROT_WRITE) < 0)
		perror("mprotect(x,y,PROT_READ | PROT_WRITE)");
# endif
}
#endif /* COGVM */

# if defined(TEST_MEMORY)

# define MBytes	*1024*1024

int
main()
{
  char *mem= sqAllocateMemory(4 MBytes, 40 MBytes);
  printf("memory allocated at %p\n", mem);
  sqShrinkMemoryBy((int)heap + heapSize, 5 MBytes);
  sqGrowMemoryBy((int)heap + heapSize, 1 MBytes);
  sqGrowMemoryBy((int)heap + heapSize, 1 MBytes);
  sqGrowMemoryBy((int)heap + heapSize, 1 MBytes);
  sqGrowMemoryBy((int)heap + heapSize, 100 MBytes);
  sqShrinkMemoryBy((int)heap + heapSize, 105 MBytes);
  return 0;
}

# endif /* defined(TEST_MEMORY) */
#endif /* !SPURVM */
