/*
 * mm-implicit.c - an empty malloc package
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 * @id : 201702012
 * @name : 박상현
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */


/* MACROS */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)
#define OVERHEAD 8
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define PACK(size,alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int*) (p))
#define PUT(p, val) (*(unsigned int*) (p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))	//	need for realloc
#define SIZE_PTR(p)  ((size_t*)(((char*)(p)) - SIZE_T_SIZE))	//	need for realloc

/* End of MACROS */


static char *heap_listp = 0;	//	pointer of first block.
static char *next_listp = 0;


/*
 * coalesce
 */
void *coalesce(void *bp){

	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));	//	이전 블럭의 할당 여부 0 = NO, 1 = YES
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));	//	다음 블럭의 할당 여부 0 = NO, 1 = YES
	size_t size = GET_SIZE(HDRP(bp));	//	현재 블럭의 크기

	//	case 1 : both side allocs == 1, no coalescing. return bp.
	if(prev_alloc && next_alloc){
		//		return bp;
	}

	//	case 2: prev_alloc == 1, next_alloc == 0. coalesce the next block and return bp.
	else if(prev_alloc && !next_alloc){	
		size = size + GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));	//	why????????????????
	}

	//	case 3: prev_alloc == 0, next_alloc == 1. coalesce the prev block and return bp.
	else if(!prev_alloc && next_alloc){
		size = size + GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));

		bp = PREV_BLKP(bp);
	}

	//	case 4: prev_alloc == 0, next_alloc == 0. coalesce prev, present, next block. return bp.
	else{
		size = size + GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}

	next_listp = bp;	//	give the pointer of current block payload to 'next_listp' for next-fit search!

	return bp;
}

/*
 * place
 */
static void place(void *bp, size_t asize){

	size_t csize = GET_SIZE(HDRP(bp));
	if((csize - asize) >= (2 * DSIZE)){	//	if size is more enough, place and make the rests a 'free block'!
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0));
	}
	else{	//	if the difference between current_size and asize < 16bytes, just place!
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
}

/*
 * extend_heap
 */
static void *extend_heap(size_t words){

	char *bp;
	size_t size;

	/* Allocate an even number of words to maintain alignment */	//	double word alignment!
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;	//	size : byte단위
	if((long)(bp = mem_sbrk(size)) < 0)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0));	//	Free block header
	PUT(FTRP(bp), PACK(size, 0));	//	Free block footer
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));	//	New epilogue header

	/* Coalesce if the previous block was free */
	return coalesce(bp);
}

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {

	if( (heap_listp = mem_sbrk(4 * WSIZE) ) == NULL)	//	초기 empty heap 생성
		return -1;	//	heap_listp = 새로 생성되는 heap 영역의 시작 주소

	PUT(heap_listp, 0);	//	정렬을 위해서 의미없는 값(4byte)을 삽입
	PUT(heap_listp + WSIZE, PACK(OVERHEAD, 1));	//	prologue header
	PUT(heap_listp + DSIZE, PACK(OVERHEAD, 1));	//	prologue footer
	PUT(heap_listp + WSIZE + DSIZE, PACK(0, 1));	//	epilogue header
	heap_listp += DSIZE;
	next_listp = heap_listp;

	if((extend_heap(CHUNKSIZE / WSIZE)) == NULL){	//	CHUNKSIZE 바이트의 free block 만큼 empty heap을 확장
		//  생성된 empty heap을 free block으로 확장
		return -1;
	}	//	CHUNKSIZE는 1 << 12 임.
	return 0;
}

/*
 * find_fit
 */
static void *find_fit(size_t asize){

	void *bp;
	void *t = next_listp + WSIZE;	//	next_listp를 프롤로그에서free블록으로  이동시킨다.

	for(bp = next_listp; bp != t; bp = NEXT_BLKP(bp)){	//	bp가 t까지 갈 때 까지 탐색
		if( !GET_SIZE(HDRP(bp)) && GET_ALLOC(HDRP(bp))){	//	에필로그를 찾았으면
			bp = PREV_BLKP(heap_listp);	//	heap_listp는 프롤로그의 footer를 가리킴.
										//	bp는 그 전블럭인 프롤로그의 header를 가리키게됨.
			t = next_listp;				//	t는 프롤로그 footer, 즉 next_listp를 가리키게 됨.
		}
		else if( !GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
			next_listp = bp;	//	alloc되어있지 않고, size가 충분한 공간을 찾으면, 
			return bp;			//	next_listp에 해당 포인터를 저장해주고, 리턴한다.
		}
	}
	return NULL;
}

/*
 * malloc
 */
void *malloc (size_t size) {
	size_t asize;	//	Adjusted block size
	size_t extendsize;	//	Amount to extend heap if no fit
	char *bp;

	/* Ignore spurious requests */
	if(size <= 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if(size <= DSIZE){
		asize = 2 * DSIZE;
	}else{
		asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
	}
	/* Search the free list for a fit */
	if((bp = find_fit(asize)) != NULL){
		place(bp, asize);
		return bp;
	}

	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize, CHUNKSIZE);
	if((bp = extend_heap(extendsize / WSIZE)) == NULL)
		return NULL;

	place(bp, asize);
	return bp;
}

/*
 * free
 */
void free (void *ptr) {
	if(!ptr) return;	//	잘못된 free 요청인 경우 함수를 종료한다. 이전 프로시져로 return
	size_t size = GET_SIZE(HDRP(ptr));	//	bp의 헤더에서 block size를 읽어온다.

	//	실제로 데이터를 지우는 것이 아니라
	//	header와 footer의 최하위 1 bit (1, 할당된 상태) 만을 수정

	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));	//	header와 footer는 같은 것이기 때문에 size도 같음.

	coalesce(ptr);	//	주위에 빈 블록이 있을 시 병합한다.
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {	//	same as mm-naive.c


	size_t oldsize;
	void *newptr;

	/* If size == 0 then this is just free, and we return NULL. */
	if(size == 0) {
		free(oldptr);
		return 0;
	}

	/* If oldptr is NULL, then this is just malloc. */
	if(oldptr == NULL) {
		return malloc(size);
	}

	newptr = malloc(size);

	/* If realloc() fails the original block is left untouched  */
	if(!newptr) {
		return 0;
	}

	/* Copy the old data. */
	oldsize = *SIZE_PTR(oldptr);
	if(size < oldsize) oldsize = size;
	memcpy(newptr, oldptr, oldsize);

	/* Free the old block. */
	free(oldptr);

	return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
	return NULL;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
	return p < mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
	return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int verbose) {
}
