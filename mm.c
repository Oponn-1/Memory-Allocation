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

static const uint64_t HEADER_SIZE = 8;
static const uint64_t FOOTER_SIZE = 8;
static const size_t MAX_SIZE = 4096;
static const uint64_t MIN_BLOCK_SIZE = 32;

static char* heapStart = 0;
//static char* prologuePointer = 0;
static char* epiloguePointer = 0;
//static char* nextFit;
static char* listTail = 0;

static void *addToHeap(size_t words);
static void put(void *bp, size_t asize);
static void *fit(size_t asize);
static void *coalesce(void *bp);

struct freeBlockPointers {
	uint64_t previous;
	uint64_t next;
};

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
	return((char*)(p) - HEADER_SIZE);
}

static inline char* getFooter(void* p)
{
	return((char*)(p) + getSize(getHeader(p)) - ALIGNMENT);
}

static inline char* getNext(void* p)
{
	return((char*)(p) + getSize(((char*)(p) - HEADER_SIZE)));
}

static inline char* getPrevious(void* p)
{
	return((char*)(p) - getSize(((char*)(p) - ALIGNMENT)));
}

static inline uint64_t getListPointerPrev(void* p)
{
	struct freeBlockPointers pointerStruct = * ((struct freeBlockPointers *)(p));
	return pointerStruct.previous;
}

static inline uint64_t getListPointerNext(void* p)
{
	struct freeBlockPointers pointerStruct = * ((struct freeBlockPointers *)(p));
	return pointerStruct.next;
}

static inline void setListPointerPrev(void* p, void* newPrev)
{
	(*(uint64_t*)(p) = (uint64_t) newPrev);
}

static inline void setListPointerNext(void* p, void* newNext)
{
	uint64_t* nextField = (uint64_t*)((char*)(p) + HEADER_SIZE);
	*(nextField) = (uint64_t) newNext;
}

static void deleteListNode(void* p)
{
	char* next = (char*)getListPointerNext(p);
	char* prev = (char*)getListPointerPrev(p);
	setListPointerNext(prev, next);
	setListPointerPrev(next, prev);
}

static void addListNode(void *p)
{
	struct freeBlockPointers listPointers; 
	listPointers.previous = (uint64_t) listTail;
	listPointers.next = 0;
	if (listTail != heapStart) {
		setListPointerNext(listTail, p);
	}
	(*(struct freeBlockPointers*)(p) = listPointers);
	listTail = p;
}

static void *coalesce(void *bp) 
{
    size_t previousIsAllocated = getAllocated(getFooter(getPrevious(bp)));
    size_t nextIsAllocated = getAllocated(getHeader(getNext(bp)));
    size_t size = getSize(getHeader(bp));
    struct freeBlockPointers listPointers;

    if (previousIsAllocated && nextIsAllocated) {			/* Case 1 */
    	//addListNode(bp);
		return bp;
    }

    else if (previousIsAllocated && !nextIsAllocated) {		/* Case 2 */
    	char* next = getNext(bp);
    	/*listPointers.previous = getListPointerPrev(next);
    	listPointers.next = getListPointerNext(next);
    	if (listTail == next) {
    		listTail = bp;
    	}*/
		size += getSize(getHeader(next));
		writeData(getHeader(bp), combine(size, 0));
		writeData(getFooter(bp), combine(size,0));
    }

    else if (!previousIsAllocated && nextIsAllocated) {		/* Case 3 */
		char* prev = getPrevious(bp);
		/*listPointers.previous = getListPointerPrev(prev);
		listPointers.next = getListPointerNext(prev);
		if (listTail == bp) {
			listTail = prev;
		}*/
		size += getSize(getHeader(prev));
		writeData(getFooter(bp), combine(size, 0));
		writeData(getHeader(getPrevious(bp)), combine(size, 0));
		bp = getPrevious(bp);
    }

    else {													/* Case 4 */
		char* prev = getPrevious(bp);
		char* next = getNext(bp);
		/*deleteListNode(prev);
		listPointers.previous = getListPointerPrev(next);
		listPointers.next = getListPointerNext(next);
		if ((listTail == bp) || (listTail == next)) {
			listTail = prev;
		}*/
		size += getSize(getHeader(prev)) + getSize(getFooter(next));
		writeData(getHeader(getPrevious(bp)), combine(size, 0));
		writeData(getFooter(getNext(bp)), combine(size, 0));
		bp = getPrevious(bp);
    }

    (*(struct freeBlockPointers*)(bp) = listPointers);


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


static void* addToHeap(size_t bytes) 
{
    char *bp;
    size_t size= align(bytes);

    bp = mem_sbrk(size);

	if ((long)(bp) == -1) {
		printf("%s\n", "MEM_SBRK FAILED");
		return NULL;
	}                                       //line:vm:mm:endextend

    /* Initialize free block header/footer and the epilogue header */
    writeData(getHeader(bp), combine(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
    writeData(getFooter(bp), combine(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
    writeData(getHeader(getNext(bp)), combine(0, 1));   /* New epilogue header */ //line:vm:mm:newepihdr
    epiloguePointer = (getNext(bp));

    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          //line:vm:mm:returnblock
}


static void put(void* p, size_t putSize)
{
	size_t blockSize = getSize(getHeader(p));

	if ((blockSize - putSize) >= (MIN_BLOCK_SIZE))
	{
		writeData(getHeader(p), combine(putSize, 1));
		writeData(getFooter(p), combine(putSize, 1));
		p = getNext(p);
		writeData(getHeader(p), combine(blockSize - putSize, 0));
		writeData(getFooter(p), combine(blockSize - putSize, 0));
	}
	else {
		writeData(getHeader(p), combine(blockSize, 1));
		writeData(getFooter(p), combine(blockSize, 1));
	}
}

static void* findFit(size_t putSize)
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

	//for (bp = listTail; bp != heapStart; bp = )

    for (bp = heapStart; getSize(getHeader(bp)) > 0; bp = getNext(bp)) {
		if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
		    return bp;
		}
    }
    return NULL;
}



bool mm_init(void) 
{
    /* Create the initial empty heap */
    if ((heapStart = mem_sbrk(4*HEADER_SIZE)) == (void *)-1){ //line:vm:mm:begininit
		return false;
	}
    writeData(heapStart, 0);                          /* Alignment padding */
    writeData(heapStart + (1*HEADER_SIZE), combine(ALIGNMENT, 1)); /* Prologue header */ 
    writeData(heapStart + (2*HEADER_SIZE), combine(ALIGNMENT, 1)); /* Prologue footer */ 
    writeData(heapStart + (3*HEADER_SIZE), combine(0, 1));     /* Epilogue header */
    heapStart += (2*HEADER_SIZE);                     //line:vm:mm:endinit  
    listTail = heapStart;
    printf("%p\n", heapStart );
/* $end mminit */

//#ifdef NEXT_FIT
    //nextFit = heapStart;
//#endif
/* $begin mminit */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (addToHeap(MAX_SIZE) == NULL) {
    	printf("%s\n", "FAILED TO EXTEND HEAP WHEN INITIALIZING");
		return false;
    }

    mm_checkheap(336);
    return true;
}




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

    if ((p = findFit(alignedSize)) != NULL) {
    	put(p, alignedSize);
    	return p;
    }

    extraNeededSpace = max(alignedSize, MAX_SIZE);
    if ((p = addToHeap(extraNeededSpace)) == NULL)
    {
    	return NULL;
    }

    put(p, alignedSize);

    return p;
}



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
    void *bp = heapStart;
    void *prev = NULL;
    bool somethingWrong = false;

    while (bp != prev) {
        if (!aligned(bp)){
            printf("%s: %p\n", "NOT ALIGNED", bp);
            somethingWrong = true;
        }
        if (!in_heap(bp)){
            printf("%s: %p\n", "NOT IN HEAP", bp);
            somethingWrong = true;
        }
            
        if (prev != NULL){
            if (getAllocated(prev) == 0 && getAllocated(bp) == 0){
                printf("%s: %p and %p\n", "COLESCE FAILED", prev, bp);
                somethingWrong = true;
            }
                
        }

        prev = bp;
        bp = getNext(bp);
    }

    if (!somethingWrong) {
	    printf("%s\n", "VALID");
	}


#endif /* DEBUG */
    return true;
}


