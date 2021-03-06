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
//#define DEBUG

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
static const size_t MAX_SIZE = 2048;
static const uint64_t MIN_BLOCK_SIZE = 32;

static const uint64_t SEGMENT_BOUND_1 = 32;
static const uint64_t SEGMENT_BOUND_2 = 64;
static const uint64_t SEGMENT_BOUND_3 = 128;
static const uint64_t SEGMENT_BOUND_4 = 256;
static const uint64_t SEGMENT_BOUND_5 = 512;

static char* heapStart = 0;
static char* list_2_start = 0;
static char* list_3_start = 0;
static char* list_4_start = 0;
static char* list_5_start = 0;
static char* list_6_start = 0;
//static char* prologuePointer = 0;
static char* epiloguePointer = 0;
//static char* nextFit;
static char* listTail = 0;
static char* listTail_2 = 0;
static char* listTail_3 = 0;
static char* listTail_4 = 0;
static char* listTail_5 = 0;
static char* listTail_6 = 0;

static char* nextFit_1 = 0;
static char* nextFit_2 = 0;
static char* nextFit_3 = 0;
static char* nextFit_4 = 0;
static char* nextFit_5 = 0;
static char* nextFit_6 = 0;

static void *addToHeap(size_t words);
static void put(void *bp, size_t asize);
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

static inline uint64_t getPreviousAllocated(void*p)
{
	return (getData(p) & 0x2);
}

static inline uint64_t combineWithPreviousAllocated(uint64_t data, uint64_t previousAllocated)
{
	return (data | previousAllocated);
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

static inline void* getListPointerPrev(void* p)
{
	return *(void **)(p);
}

static inline void* getListPointerNext(void* p)
{
	return *(void **)(p + 8);
}

static inline void setListPointerPrev(void* p, void* newPrev)
{
	*(void **)(p) = newPrev;
}

static inline void setListPointerNext(void* p, void* newNext)
{
	*(void **)(p + 8) = newNext;
}

static void deleteListNode(void* p)
{
	void* prev = getListPointerPrev(p);

	if (nextFit_1 == p) {
		nextFit_1 = prev;
	} else if (nextFit_2 == p) {
		nextFit_2 = prev;
	} else if (nextFit_3 == p) {
		nextFit_3 = prev;
	} else if (nextFit_4 == p) {
		nextFit_4 = prev;
	} else if (nextFit_5 == p) {
		nextFit_5 = prev;
	} else if (nextFit_6 == p) {
		nextFit_6 = prev;
	}

	if (listTail == p) {
		setListPointerNext(prev, NULL);
		listTail = prev;
	} else if (listTail_2 == p) {
		setListPointerNext(prev, NULL);
		listTail_2 = prev;
	} else if (listTail_3 == p) {
		setListPointerNext(prev, NULL);
		listTail_3 = prev;
	} else if (listTail_4 == p) {
		setListPointerNext(prev, NULL);
		listTail_4 = prev;
	} else if (listTail_5 == p) {
		setListPointerNext(prev, NULL);
		listTail_5 = prev;
	} else if (listTail_6 == p) {
		setListPointerNext(prev, NULL);
		listTail_6 = prev;
	} else {
		setListPointerPrev(getListPointerNext(p), getListPointerPrev(p));
		setListPointerNext(getListPointerPrev(p), getListPointerNext(p));
	}
	
}

static void addListNode(void *p, uint64_t size)
{
	char* targetTail = 0;
	if (size > SEGMENT_BOUND_5)  {
		targetTail = listTail_6;
	}
	if (size <= SEGMENT_BOUND_5) {
		targetTail = listTail_5;
	}
	if (size <= SEGMENT_BOUND_4) {
		targetTail = listTail_4;
	}
	if (size <= SEGMENT_BOUND_3) {
		targetTail = listTail_3;
	}
	if (size <= SEGMENT_BOUND_2) {
		targetTail = listTail_2;
	} 
	if (size <= SEGMENT_BOUND_1) {
		targetTail = listTail;
	}
	setListPointerNext(targetTail, p);
	setListPointerPrev(p, targetTail);
	setListPointerNext(p, NULL);
	if (targetTail == listTail) {
		listTail = p;
	} else if (targetTail == listTail_2) {
		listTail_2 = p;
	} else if (targetTail == listTail_3) {
		listTail_3 = p;
	} else if (targetTail == listTail_4) {
		listTail_4 = p;
	} else if (targetTail == listTail_5) {
		listTail_5 = p;
	} else if (targetTail == listTail_6) {
		listTail_6 = p;
	}
}

static void *coalesce(void *bp) 
{
    uint64_t previousIsAllocated = getAllocated(getFooter(getPrevious(bp)));
    uint64_t nextIsAllocated = getAllocated(getHeader(getNext(bp)));
    uint64_t size = getSize(getHeader(bp));

    if (previousIsAllocated && nextIsAllocated) {			/* Case 1 */
    	addListNode(bp, size);
		return bp;
    }

    else if (previousIsAllocated && !nextIsAllocated) {		/* Case 2 */
    	char* next = getNext(bp);

    	deleteListNode(next);

		size += getSize(getHeader(next));
		writeData(getHeader(bp), combine(size, 0));
		writeData(getFooter(bp), combine(size,0));

		addListNode(bp, getSize(getHeader(bp)));
    }

    else if (!previousIsAllocated && nextIsAllocated) {		/* Case 3 */
		char* prev = getPrevious(bp);

		deleteListNode(prev);

		size += getSize(getHeader(prev));
		writeData(getFooter(bp), combine(size, 0));
		writeData(getHeader(getPrevious(bp)), combine(size, 0));
		bp = getPrevious(bp);

		addListNode(bp, getSize(getHeader(bp)));
    }

    else {													/* Case 4 */
		char* prev = getPrevious(bp);
		char* next = getNext(bp);

		deleteListNode(next);
		deleteListNode(prev);

		size += getSize(getHeader(prev)) + getSize(getFooter(next));
		writeData(getHeader(getPrevious(bp)), combine(size, 0));
		writeData(getFooter(getNext(bp)), combine(size, 0));
		bp = getPrevious(bp);

		addListNode(bp, getSize(getHeader(bp)));
    }


    //mm_checkheap(__LINE__);
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
		deleteListNode(p);

		writeData(getHeader(p), combine(putSize, 1));
		writeData(getFooter(p), combine(putSize, 1));
		
		p = getNext(p);
		
		writeData(getHeader(p), combine(blockSize - putSize, 0));
		writeData(getFooter(p), combine(blockSize - putSize, 0));

		addListNode(p, getSize(getHeader(p)));
	}
	else {
		deleteListNode(p);

		writeData(getHeader(p), combine(blockSize, 1));
		writeData(getFooter(p), combine(blockSize, 1));
	}
}

static void* findFit(size_t putSize)
{
	void *bp;

	if (putSize <= SEGMENT_BOUND_1){
		for (bp = listTail; bp != heapStart; bp = getListPointerPrev(bp)) {
			if ((putSize <= getSize(getHeader(bp)))) {
				return bp;
			}
		}
		/*for (bp = nextFit_1; bp != heapStart; bp = getListPointerPrev(bp)) {
			if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
				nextFit_1 = getListPointerPrev(bp);
				return bp;
			}
		}
		for (bp = listTail; bp != nextFit_1; bp = getListPointerPrev(bp)) {
			if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
				nextFit_1 = getListPointerPrev(bp);
				return bp;
			}
		}*/
	}
	if (putSize <= SEGMENT_BOUND_2) {
		for (bp = listTail_2; bp != list_2_start; bp = getListPointerPrev(bp)) {
			if ((putSize <= getSize(getHeader(bp)))) {
				return bp;
			}
		}
		/*for (bp = nextFit_2; bp != list_2_start; bp = getListPointerPrev(bp)) {
			if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
				nextFit_2 = getListPointerPrev(bp);
				return bp;
			}
		}
		for (bp = listTail_2; bp != nextFit_2; bp = getListPointerPrev(bp)) {
			if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
				nextFit_2 = getListPointerPrev(bp);
				return bp;
			}
		}*/
	}
	if (putSize <= SEGMENT_BOUND_3) {
		for (bp = listTail_3; bp != list_3_start; bp = getListPointerPrev(bp)) {
			if ((putSize <= getSize(getHeader(bp)))) {
				return bp;
			}
		}
		/*for (bp = nextFit_3; bp != list_3_start; bp = getListPointerPrev(bp)) {
			if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
				nextFit_3 = getListPointerPrev(bp);
				return bp;
			}
		}
		for (bp = listTail_3; bp != nextFit_3; bp = getListPointerPrev(bp)) {
			if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
				nextFit_3 = getListPointerPrev(bp);
				return bp;
			}
		}*/
	}
	if (putSize <= SEGMENT_BOUND_4) {
		/*for (bp = listTail_4; bp != list_4_start; bp = getListPointerPrev(bp)) {
			if ((putSize <= getSize(getHeader(bp)))) {
				return bp;
			}
		}*/
		for (bp = nextFit_4; bp != list_4_start; bp = getListPointerPrev(bp)) {
			if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
				nextFit_4 = getListPointerPrev(bp);
				return bp;
			}
		}
		for (bp = listTail_4; bp != nextFit_4; bp = getListPointerPrev(bp)) {
			if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
				nextFit_4 = getListPointerPrev(bp);
				return bp;
			}
		}
	}
	if (putSize <= SEGMENT_BOUND_5) {
		/*for (bp = listTail_5; bp != list_5_start; bp = getListPointerPrev(bp)) {
			if ((putSize <= getSize(getHeader(bp)))) {
				return bp;
			}
		}*/
		for (bp = nextFit_5; bp != list_5_start; bp = getListPointerPrev(bp)) {
			if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
				nextFit_5 = getListPointerPrev(bp);
				return bp;
			}
		}
		for (bp = listTail_5; bp != nextFit_5; bp = getListPointerPrev(bp)) {
			if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
				nextFit_5 = getListPointerPrev(bp);
				return bp;
			}
		}
	}
	/*for (bp = listTail_6; bp != list_6_start; bp = getListPointerPrev(bp)) {
		if (!getAllocated(getHeader(bp)) && (putSize <= getSize(getHeader(bp)))) {
			return bp;
		}
	}*/
	for (bp = nextFit_6; bp != list_6_start; bp = getListPointerPrev(bp)) {
		if ((putSize <= getSize(getHeader(bp)))) {
			nextFit_6 = getListPointerPrev(bp);
			return bp;
		}
	}
	for (bp = listTail_6; bp != nextFit_6; bp = getListPointerPrev(bp)) {
		if ((putSize <= getSize(getHeader(bp)))) {
			nextFit_6 = getListPointerPrev(bp);
			return bp;
		}
	}

    return NULL;
}



bool mm_init(void) 
{
    /* Create the initial empty heap */
    if ((heapStart = mem_sbrk(16*HEADER_SIZE)) == (void *)-1){ 
		return false;
	}
    writeData(heapStart, 0);                          /* Alignment padding */
    writeData(heapStart + (1*HEADER_SIZE), combine(112, 1)); /* Prologue header */ 
    writeData(heapStart + (14*HEADER_SIZE), combine(112, 1)); /* Prologue footer */ 
    writeData(heapStart + (15*HEADER_SIZE), combine(0, 1));     /* Epilogue header */
    
    heapStart += (2*HEADER_SIZE); 
    list_2_start = heapStart + (2*HEADER_SIZE);
    list_3_start = heapStart + (4*HEADER_SIZE); 
    list_4_start = heapStart + (6*HEADER_SIZE);
    list_5_start = heapStart + (8*HEADER_SIZE);
    list_6_start = heapStart + (10*HEADER_SIZE);

    //printf("%s\n", "PASSED LIST STARTS");                    

    listTail = heapStart;
    listTail_2 = list_2_start;
    listTail_3 = list_3_start;
    listTail_4 = list_4_start;
    listTail_5 = list_5_start;
    listTail_6 = list_6_start;

    nextFit_1 = heapStart;
    nextFit_2 = list_2_start;
    nextFit_3 = list_3_start;
    nextFit_4 = list_4_start;
    nextFit_5 = list_5_start;
    nextFit_6 = list_6_start;

    //printf("%s\n", "PASSED LIST TAILS");

    setListPointerNext(listTail, NULL);
    setListPointerPrev(listTail, NULL);

    //printf("%s\n", "PASSED LISTTAIL POINTERS");

    setListPointerPrev(listTail_2, NULL);
    setListPointerNext(listTail_2, NULL);

    //printf("%s\n", "PASSED LISTTAIL 2 POINTERS");

    setListPointerPrev(listTail_3, NULL);
    setListPointerNext(listTail_3, NULL);

    //printf("%s\n", "PASSED LISTTAIL 3 POINTERS");

    setListPointerPrev(listTail_4, NULL);
    setListPointerNext(listTail_4, NULL);

    setListPointerPrev(listTail_5, NULL);
    setListPointerNext(listTail_5, NULL);

    setListPointerPrev(listTail_6, NULL);
    setListPointerNext(listTail_6, NULL);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (addToHeap(MAX_SIZE) == NULL) {
    	printf("%s\n", "FAILED TO EXTEND HEAP WHEN INITIALIZING");
		return false;
    }

    //mm_checkheap(__LINE__);
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
    	alignedSize = MIN_BLOCK_SIZE;
    }
    else
    {
    	alignedSize = align(size + ALIGNMENT);
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

    /*printf("%s\n", "LIST CHECK:");
    while (bp != listTail) {
    	if (bp == heapStart) {
    		;
    	} else {
	    	if (getAllocated(bp)) {
	    		printf("%s\n", "ALLOCATED BLOCK IN FREE LIST");
	    		somethingWrong = true;
	    	}
	    	if (bp != getListPointerPrev(getListPointerNext(bp))){
	    		printf("%s\n", "DISCONTINUOUS LIST");
	    		somethingWrong = true;
	    	}
	    }

    	bp = getListPointerNext(bp);
    }

    if (bp == listTail) {
    	if (getListPointerNext(bp) != NULL) {
    		printf("%s\n", "LIST TAIL NOT FOLLOWED BY NULL");
    		somethingWrong = true;
    	}
    }*/

	bp = heapStart;

    //printf("%s\n", "GENERAL CHECK");
    while (bp != prev && bp != epiloguePointer) {

    	//check allocation of prologue and epilogue
    	if (!getAllocated(getHeader(epiloguePointer))) {
    		printf("%s\n", "EPILOGUE NOT ALLOCATED");
    		somethingWrong = true;
    	}
    	if (!getAllocated(getHeader(heapStart))) {
    		printf("%s\n", "PROLOGUE NOT ALLOCATED");
    		somethingWrong = true;
    	}

    	//check alignment
        if (!aligned(bp)){
            printf("%s: %p\n", "NOT ALIGNED", bp);
            somethingWrong = true;
        }

        //check for out of heap range
        if (!in_heap(bp)){
            printf("%s: %p\n", "NOT IN HEAP", bp);
            somethingWrong = true;
        }
        
        //check for adjacent free blocks    
        if (prev != NULL){
            if (!getAllocated(getHeader(prev)) && !getAllocated(getHeader(bp))){
                printf("%s: %p and %p\n", "COLESCE FAILED", prev, bp);
                somethingWrong = true;
            }
                
        }

        prev = bp;
        bp = getNext(bp);
    }

    if (!somethingWrong) {
	    printf("%s: %d\n", "VALID", lineno);
	}


#endif /* DEBUG */
    return true;
}


