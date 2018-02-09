/*
 * mm.c
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*
 * If you want debugging output, uncomment the following. Be sure not
 * to have debugging enabled in your final submission
 */
#define DEBUG

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#define dbg_assert(...)
#endif /* DEBUG */

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* DRIVER */

/* What is the correct alignment? */
#define ALIGNMENT 16

static const uint64_t HALF_ALIGNMENT = 8;
static const size_t MAX_SIZE = 4096;

static char* heapStart = 0;
//static char* nextFit;

static void *addToHeap(size_t words);
static void move(void *bp, size_t asize);
static void *fit(size_t asize);
static void *coalesce(void *bp);

/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

static inline size_t max(size_t x, size_t y)
{
	return ((x) > (y)? (x) : (y)) ;
}

static inline uint64_t getData(void* p)
{
	return (*(uint64_t *)(p));
}

static inline void writeData(void* p, uint64_t data)
{
	(*(uint64_t *)(p) = (data));
}

static inline uint64_t getSize(void* p)
{
	return (getData(p) & ~0xf);
}

static inline uint64_t getAllocated(void* p)
{
	return (getData(p) & 0x1);
}

static inline uint64_t combine(uint64_t size, uint64_t isAllocated)
{
	return (size | isAllocated);
}

static inline char* getHeader(void* p)
{
	return((char*)(p) - HALF_ALIGNMENT);
}

static inline char* getFooter(void* p)
{
	return((char*)(p) + getSize(getHeader(p)) - ALIGNMENT);
}

static inline char* getNext(void* p)
{
	return((char*)(p) + getSize(((char*)(p) - HALF_ALIGNMENT)));
}

static inline char* getPrevious(void* p)
{
	return((char*)(p) - getSize(((char*)(p) - ALIGNMENT)));
}

/*static void* coalesce(void* p) 
{
	size_t previous = getAllocated(getFooter(getPrevious(p)));
	size_t next = getAllocated(getHeader(getNext(p)));
	size_t size = getSize(getHeader(p));

	if (previous && next)
	{
		return p;
	} 
	else if (previous && !next) 
	{
		size = size + getSize(getHeader(getNext(p)));
		writeData(getHeader(p), combine(size, 0));
		writeData(getFooter(p), combine(size, 0));
	}
	else if (!previous && next)
	{
		size = size + getSize(getFooter(getPrevious(p)));
		writeData(getFooter(p), combine(size, 0));
		writeData(getHeader(getPrevious(p)), combine(size, 0));
		p = getPrevious(p);
	}
	else
	{
		size = size + getSize(getFooter(getPrevious(p))) + getSize(getHeader(getNext(p)));
		writeData(getHeader(getPrevious(p)), combine(size, 0));
		writeData(getFooter(getNext(p)), combine(size, 0));
		p = getPrevious(p);
	}

	if ((nextFit > (char*)p) && (nextFit < getNext(p)))
	{
		nextFit = p;
	}

	return p;
}*/

static void *coalesce(void *bp) 
{
    size_t prev_alloc = getAllocated(getFooter(getPrevious(bp)));
    size_t next_alloc = getAllocated(getHeader(getNext(bp)));
    size_t size = getSize(getHeader(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
	return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
		size += getSize(getHeader(getNext(bp)));
		writeData(getHeader(bp), combine(size, 0));
		writeData(getFooter(bp), combine(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
		size += getSize(getHeader(getPrevious(bp)));
		writeData(getFooter(bp), combine(size, 0));
		writeData(getHeader(getPrevious(bp)), combine(size, 0));
		bp = getPrevious(bp);
    }

    else {                                     /* Case 4 */
	size += getSize(getHeader(getPrevious(bp))) + 
	    getSize(getFooter(getNext(bp)));
		writeData(getHeader(getPrevious(bp)), combine(size, 0));
		writeData(getFooter(getNext(bp)), combine(size, 0));
		bp = getPrevious(bp);
    }
/* $end mmfree */
//#ifdef NEXT_FIT
    /* Make sure the rover isn't pointing into the free block */
    /* that we just coalesced */
    //if ((nextFit > (char *)bp) && (nextFit < getNext(bp))) 
	//nextFit = bp;
//#endif
/* $begin mmfree */
    return bp;
}

/*static void* addToHeap(size_t bytes)
{
	char* p;
	size_t alignedBytes = align(bytes);

	p = mem_sbrk(alignedBytes);
	if ((long)(p) == -1) {
		return NULL;
	}

	writeData(getHeader(p), combine(alignedBytes, 0));
	writeData(getFooter(p), combine(alignedBytes, 0));
	writeData(getHeader(getNext(p)), combine(0,1));

	return coalesce(p);
}*/

static void* addToHeap(size_t bytes) 
{
    char *bp;
    size_t size= align(bytes);

    /* Allocate an even number of words to maintain alignment */
    bp = mem_sbrk(size);
	if ((long)(bp) == -1) {
		return NULL;
	}                                       //line:vm:mm:endextend

    /* Initialize free block header/footer and the epilogue header */
    writeData(getHeader(bp), combine(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
    writeData(getFooter(bp), combine(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
    writeData(getHeader(getNext(bp)), combine(0, 1)); /* New epilogue header */ //line:vm:mm:newepihdr

    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          //line:vm:mm:returnblock
}


static void move(void* p, size_t alignedSize)
{
	size_t blockSize = getSize(getHeader(p));

	if ((blockSize - alignedSize) >= (2*ALIGNMENT))
	{
		writeData(getHeader(p), combine(alignedSize, 1));
		writeData(getFooter(p), combine(alignedSize, 1));
		p = getNext(p);
		writeData(getHeader(p), combine(blockSize - alignedSize, 0));
		writeData(getFooter(p), combine(blockSize - alignedSize, 0));
	}
	else {
		writeData(getHeader(p), combine(blockSize, 1));
		writeData(getFooter(p), combine(blockSize, 1));
	}
}

static void* fit(size_t alignedSize)
{
	/*char* previousFit = nextFit;

	for (; getSize(getHeader(nextFit)) > 0; nextFit = getNext(nextFit))
	{
		if (!getAllocated(getHeader(nextFit)) && (alignedSize <= getSize(getHeader(nextFit))))
		{
			return nextFit;
		}
	}

	for(nextFit = heapStart; nextFit < previousFit; nextFit = getNext(nextFit))
	{
		if (!getAllocated(getHeader(nextFit)) && (alignedSize <= getSize(getHeader(nextFit))))
		{
			return nextFit;
		}
	}

	return NULL;*/
	void *bp;

    for (bp = heapStart; getSize(getHeader(bp)) > 0; bp = getNext(bp)) {
	if (!getAllocated(getHeader(bp)) && (alignedSize <= getSize(getHeader(bp)))) {
	    return bp;
	}
    }
    return NULL;
}


/*
 * Initialize: return false on error, true on success.
 */
/*bool mm_init(void)
{
    heapStart = mem_sbrk(4 * HALF_ALIGNMENT);
    if (heapStart == (void*)-1)
    {
    	return -1;
    }
    writeData(heapStart, 0);
    writeData(heapStart + (1 * HALF_ALIGNMENT), combine(ALIGNMENT, 1));
    writeData(heapStart + (2 * HALF_ALIGNMENT), combine(ALIGNMENT, 1));
    writeData(heapStart + (3 * HALF_ALIGNMENT), combine(0,1));
    heapStart = heapStart + (2 * HALF_ALIGNMENT);

    nextFit = heapStart;

    if (addToHeap(MAX_SIZE) == NULL)
    {
    	return -1;
    }

    return 0;
}*/

bool mm_init(void) 
{
    /* Create the initial empty heap */
    if ((heapStart = mem_sbrk(4*HALF_ALIGNMENT)) == (void *)-1){ //line:vm:mm:begininit
		return false;
	}
    writeData(heapStart, 0);                          /* Alignment padding */
    writeData(heapStart + (1*HALF_ALIGNMENT), combine(ALIGNMENT, 1)); /* Prologue header */ 
    writeData(heapStart + (2*HALF_ALIGNMENT), combine(ALIGNMENT, 1)); /* Prologue footer */ 
    writeData(heapStart + (3*HALF_ALIGNMENT), combine(0, 1));     /* Epilogue header */
    heapStart += (2*HALF_ALIGNMENT);                     //line:vm:mm:endinit  
/* $end mminit */

//#ifdef NEXT_FIT
    //nextFit = heapStart;
//#endif
/* $begin mminit */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (addToHeap(MAX_SIZE) == NULL) {
		return false;
    }
    return true;
}

/*
 * malloc
 */
void* malloc(size_t size)
{
    size_t alignedSize;
    size_t extraNeededSpace;
    char* p;

    if (heapStart == 0) 
    {
    	mm_init();
    }

    if (size == 0)
    {
    	return NULL;
    }

    if (size <= ALIGNMENT)
    {
    	alignedSize = 2* ALIGNMENT;
    }
    else
    {
    	alignedSize = ALIGNMENT * ((size + ALIGNMENT + (ALIGNMENT-1)) / ALIGNMENT);
    }

    if ((p = fit(alignedSize)) != NULL) {
    	move(p, alignedSize);
    	return p;
    }

    extraNeededSpace = max(alignedSize, MAX_SIZE);
    if ((p = addToHeap(extraNeededSpace)) == NULL)
    {
    	return NULL;
    }

    move(p, alignedSize);

    return p;
}

/*
 * free
 */
void free(void* p)
{
    if (p == 0)
	{
		return;
	}
	size_t size = getSize(getHeader(p));
	if (heapStart == 0)
	{
		mm_init();
	}
	writeData(getHeader(p), combine(size, 0));
	writeData(getFooter(p), combine(size, 0));
	coalesce(p);
}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    size_t previousSize;
    void* new;

    if(size == 0) 
    {
    	free(oldptr);
    	return 0;
    }

    if(oldptr == NULL)
    {
    	return malloc(size);
    }

    new = malloc(size);

    if(!new)
    {
    	return 0;
    }

    previousSize = getSize(getHeader(oldptr));
    if(size < previousSize)
    {
    	previousSize = size;
    }
    mem_memcpy(new, oldptr, previousSize);

    free(oldptr);

    return new;
}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */
void* calloc(size_t nmemb, size_t size)
{
    void* ret;
    size *= nmemb;
    ret = malloc(size);
    if (ret) {
        memset(ret, 0, size);
    }
    return ret;
}

/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno)
{
#ifdef DEBUG
    char* p = heapStart;

    if(verbose)
    {
    	printf("Heap (%p):\n", heapStart);
    }
    if((getSize(getHeader(heapStart)) != ALIGNMENT) || !getAllocated(getHeader(heapStart)))
    {
    	printf("Bad prologue header\n");
    }
    checkblock(heapStart);

    for(p = heapStart; getSize(getHeader(p)) > 0; p = getNext(p)) 
    {
    	if (verbose)
    	{
    		printblock(p);
    	}
    	checkblock(p);
    }

    if(verbose)
    {
    	printblock(p);
    }
    if((getSize(getHeader(p)) != 0 || !(getAllocated(getHeader(p)))))
    {
    	printf("Bad epilogue header\n");
    }
#endif /* DEBUG */
    return true;
}


static void printblock(void *bp) 
{
    /*size_t hsize, halloc, fsize, falloc;

    checkheap(0);
    hsize = getSize(getHeader(bp));
    halloc = getAllocated(getHeader(bp));  
    fsize = getSize(getFooter(bp));
    falloc = getAllocated(getFooter(bp));  

    if (hsize == 0) {
	printf("%p: EOL\n", bp);
	return;
    }

    printf("%p: header: [%p:%c] footer: [%p:%c]\n", bp, 
	hsize, (halloc ? 'a' : 'f'), 
	fsize, (falloc ? 'a' : 'f')); */
}

static void checkblock(void* p)
{
	if((size_t)(p) % 8)
	{
		printf("Error: %p is not doubleword aligned\n", p);
	}
	if(getData(getHeader(p)) != getData(getFooter(p)))
	{
		printf("Error: header does not match footer\n");
	}
}
