#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};


#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE  (4)                                      // Word        4 bytes
#define DSIZE (8)                                      // Double Word 8 bytes
#define CHUNKSIZE  (1 << 12)                                // (1 << 12) = 4096

#define MAX(x,y)   ((x >= y) ? (x) : (y))                   // max
#define MIN(x,y)   ((x <= y) ? (x) : (y))                   // min

#define PACK(size,alloc)  (size | alloc)         // 3 for alloc,29 for size

#define GET(p)              (*((int*)(p)))           // unsigned int 4 bytes
#define PUT(p,val)          ((*((int*)(p))) = val)   // put val

#define GET_SIZE(blk_ptr)        (GET((void*)blk_ptr - WSIZE) & ~0x7)        //(~0x7 = (111..29 bits) + (000..3bits)) 
#define GET_ALLOC(blk_ptr)       (GET((void*)blk_ptr - WSIZE) &  0x1)        // 0 unused 1 used
#define GET_VALID_SIZE(blk_ptr)  (GET_SIZE(blk_ptr) - 0x8)
//#define GET_PREALLOC(p) (GET(p) & 0x2)
#define HEADER_PTR(blk_ptr)      ((void*)blk_ptr - WSIZE)                          // Header 4 bytes
#define FOOTER_PTR(blk_ptr)      ((void*)blk_ptr + GET_SIZE(blk_ptr) - DSIZE)     // BLK_PTR + malloced bytes = Footer_Ptr 

#define PUT_HEADER(blk_ptr,size,alloc) PUT(HEADER_PTR(blk_ptr),PACK(size,alloc))           // FOR CONVENIENCE
#define PUT_FOOTER(blk_ptr,size,alloc) PUT(FOOTER_PTR(blk_ptr),PACK(size,alloc))           // FOR CONVENIENCE
#define PUT_PREV_PTR(blk_ptr,address) PUT(blk_ptr,address)
#define PUT_NEXT_PTR(blk_ptr,address) PUT(blk_ptr + WSIZE,address)
// #define PRED_PTR(ptr) ((void *)(ptr))
// #define SUCC_PTR(ptr) ((void *)(ptr) + WSIZE)

// #define PRED(ptr) (*(void **)(ptr))
// #define SUCC(ptr) (*(void **)(SUCC_PTR(ptr)))
// #define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))
#define NEXT_BLOCK_PTR(blk_ptr)  ((void*)blk_ptr + GET_SIZE(blk_ptr))                       // NEXT_BLOCK_PTR = BLK_PTR + SIZE
#define PREV_BLOCK_PTR(blk_ptr)  ((void*)blk_ptr - GET_SIZE((blk_ptr - WSIZE)))         // PREV_BLOCK_PTR = BLK_PTR - PREV_SIZE

#define ROUNDUP_EIGHT_BYTES(size)     (((size + (DSIZE - 1)) / 8) * 8)                 // ROUND FOR EVERY 8 BYTES

static void* heap_list = NULL;

struct block_node
{
    struct block_node* prev_ptr;
    struct block_node* next_ptr;
} block_node;

static struct blocks_list_node
{
    struct block_node start_node;
    struct block_node end_node;
} blocks_list[32];

static size_t sizet(size_t size)
{
    size_t tmpsize = 1,index = 0;
    while((tmpsize <<= 1) <= size)
        ++index;
        
    return index;
}
//inline void set_next_prealloc(void* bp, size_t prealloc);
//int mm_init(void);
//void *mm_malloc(size_t size);
//void mm_free(void *bp);
//void *mm_realloc(void *ptr, size_t size);
static void init_node(struct block_node* block_node)
{
    block_node->next_ptr = NULL;
    block_node->prev_ptr = NULL;
}

// static void *coalesce(void *bp)
//{
//     size_t prev_alloc = GET_ALLOC(FTRP(PREV(bp)));
//     size_t next_alloc = GET_ALLOC(HDRP(NEXT(bp)));
////     size_t prev_alloc = GET_PREALLOC(HDRP(bp));
////    size_t next_alloc = GET_ALLOC(HDRP(NEXT(bp)));
//    size_t size = GET_SIZE(HDRP(bp));
//    //int isPre = (pre_listp == bp);
//    if (prev_alloc && next_alloc) { // Case 1
//        pre_listp = bp;
//        return bp;
//    }
//
//     if (prev_alloc && !next_alloc) { /* Case 2 */
//         size += GET_SIZE(HDRP(NEXT(bp)));
//         PUT(HDRP(bp), PACK(size, 0));
//         PUT(FTRP(bp), PACK(size,0));
//     }
////    if (prev_alloc && !next_alloc) { /* Case 2 */
////        size += GET_SIZE(HDRP(NEXT(bp)));
////        PUT(HDRP(bp), PACK(size,1, 0));
////        PUT(FTRP(bp), PACK(size,1, 0));
////
////    }
//
//    else if (!prev_alloc && next_alloc) { /* Case 3 */
//        size += GET_SIZE(HDRP(PREV(bp)));
//         PUT(FTRP(bp), PACK(size, 0));
//         PUT(HDRP(PREV(bp)), PACK(size, 0));
////        PUT(FTRP(bp), PACK(size, 1, 0));
////        PUT(HDRP(PREV(bp)), PACK(size, 1, 0));
//        bp = PREV(bp);
//
//    }
//
//    else { /* Case 4 */
//        size += GET_SIZE(HDRP(PREV(bp))) +
//                GET_SIZE(FTRP(NEXT(bp)));
//         PUT(HDRP(PREV(bp)), PACK(size, 0));
//         PUT(FTRP(NEXT(bp)), PACK(size, 0));
////        PUT(HDRP(PREV(bp)), PACK(size,1, 0));
////        PUT(FTRP(NEXT(bp)), PACK(size,1, 0));
//        bp = PREV(bp);
//    }
//    set_next_prealloc(bp,0);
//    pre_listp = bp;
//    return bp;
//}
static void init_list(struct blocks_list_node* block_list)
{
    init_node(&block_list->start_node);
    init_node(&block_list->end_node);
    block_list->start_node.next_ptr = (&block_list->end_node);
    block_list->end_node.prev_ptr   = (&block_list->start_node); 
}

static void insert(struct blocks_list_node* block_list,void* block_ptr)
{
    PUT(block_ptr,(int)&block_list->start_node);
    PUT(block_ptr + WSIZE,(int)block_list->start_node.next_ptr);
    
    struct block_node* block_node_ptr = (struct block_node*)block_ptr;
    block_node_ptr->next_ptr = block_list->start_node.next_ptr; 
    block_node_ptr->prev_ptr = &block_list->start_node;
    block_node_ptr->prev_ptr->next_ptr = block_node_ptr;
    block_node_ptr->next_ptr->prev_ptr = block_node_ptr;
}

static void remove_block_ptr(void* blk_node)
{
    struct block_node* block_ptr = (struct block_node*)blk_node;
    block_ptr->prev_ptr->next_ptr = block_ptr->next_ptr;
    block_ptr->next_ptr->prev_ptr = block_ptr->prev_ptr;
}

/*
 * extend_heap - heap_mem needed to be larger extend_wsize bytes
 */
static void* extend_heap(size_t extend_wsize)
{
    //printf("extend begin\n");
    void* blk_ptr = NULL;
    size_t size;
    
    size = (extend_wsize % 2) ? (extend_wsize + 1) * WSIZE : extend_wsize * WSIZE;
    if((blk_ptr = mem_sbrk(size)) == (void*)-1)	
    	return NULL;
    //    prealloc = GET_PREALLOC(HDRP(bp));
    /* Initialize free block header/footer and the epilogue header */
    size_t index = sizet(size);
    PUT_HEADER(blk_ptr,size,0);
    PUT_FOOTER(blk_ptr,size,0);
    PUT(FOOTER_PTR(blk_ptr) + WSIZE,PACK(0,0));
    insert(blocks_list + index,blk_ptr);
    //printf("extend end\n");
    return blk_ptr; 
}

static void* find(size_t index,size_t size)
{
    int i;
    struct block_node* ptr,*end_ptr;
    for(i=index;i<32;++i)
    {
        ptr = blocks_list[i].start_node.next_ptr;
        end_ptr = &blocks_list[i].end_node;
        while(ptr != end_ptr)
        {
            if(GET_SIZE(((void*)ptr)) >= size)
            	return (void*)ptr;
            ptr = ptr->next_ptr;
        }
    }
    return NULL;
}

static void place(void* blk_ptr,size_t size)
{
    size_t prev_size = GET_SIZE(blk_ptr),now_size = prev_size - size;
    remove_block_ptr(blk_ptr);
    
    if(now_size <= 8)
    {
        PUT_HEADER(blk_ptr,prev_size,1);
        PUT_FOOTER(blk_ptr,prev_size,1);
        return;
    }
        
    PUT_HEADER(blk_ptr,size,1);
    PUT_FOOTER(blk_ptr,size,1);
    
    void* next_ptr = NEXT_BLOCK_PTR(blk_ptr);
    size_t index =  sizet(now_size);
    PUT_HEADER(next_ptr,now_size,0);
    PUT_FOOTER(next_ptr,now_size,0);
    insert(blocks_list + index,next_ptr);
}

static void* merge(void* ptr)
{
    // Front_block Used     Behind_block Used    - DOING NOTING
    void* prev_ptr,*next_ptr,*now_ptr,*heap_hi_address = mem_heap_hi();

    // Merge Free Front_blocks
    now_ptr = ptr;
    prev_ptr = PREV_BLOCK_PTR(now_ptr);
    while(prev_ptr > heap_list && !GET_ALLOC(prev_ptr))
    {
        size_t sum_size = GET_SIZE(now_ptr) + GET_SIZE(prev_ptr);
        PUT_FOOTER(now_ptr,sum_size,0);
        PUT_HEADER(prev_ptr,sum_size,0);
        remove_block_ptr(prev_ptr);
        now_ptr = prev_ptr;
        prev_ptr = PREV_BLOCK_PTR(prev_ptr);
    }
    
    // Merge Free Behind_blocks
    
    next_ptr = NEXT_BLOCK_PTR(ptr);
    while(next_ptr < heap_hi_address && !GET_ALLOC(next_ptr))
    {
        size_t sum_size = GET_SIZE(now_ptr) + GET_SIZE(next_ptr);
        PUT_HEADER(now_ptr,sum_size,0);
        PUT_FOOTER(next_ptr,sum_size,0);
        remove_block_ptr(next_ptr);
        next_ptr = NEXT_BLOCK_PTR(next_ptr);
    }
    return now_ptr;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_list = mem_sbrk(4 * WSIZE)) == (void*)-1)
    	return -1;
    //    PUT(heap_listp, 0); /* Alignment padding */
//    // PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
//    // PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
//    // PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* Epilogue header */
//    // heap_listp += (2*WSIZE);
//    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1, 1)); /* Prologue header */
//    PUT(heap_listp + (2*WSIZE), PACK(DSIZE,1, 1)); /* Prologue footer */
//    PUT(heap_listp + (3*WSIZE), PACK(0,1, 1)); /* Epilogue header */
//    heap_listp += DSIZE;
//    pre_listp = heap_listp;
//    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    PUT(heap_list,PACK(0,0));
    PUT(heap_list + 1 * WSIZE,PACK(8,1)); // first two blocks start (8/1)
    PUT(heap_list + 2 * WSIZE,PACK(8,1)); // first two blocks end   (8/1)
    PUT(heap_list + 3 * WSIZE,PACK(0,1)); // end one block    end   (0/1)
    heap_list += (2 * WSIZE);    // heap_list = (8/1)
    
    int i;
    for(i=0;i<32;++i)
    	init_list(blocks_list + i);
    
    if(extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

// static void place(void *bp, size_t asize)
// {
//     size_t size = GET_SIZE(HDRP(bp));

//     if ((size - asize) >= (2*DSIZE)) {
//         PUT(HDRP(bp),PACK(asize,1));
//         PUT(FTRP(bp),PACK(asize,1));
//         PUT(HDRP(NEXT(bp)),PACK(size - asize,0));
//         PUT(FTRP(NEXT(bp)),PACK(size - asize,0));
//     } else {
//         PUT(HDRP(bp),PACK(size,1));
//         PUT(FTRP(bp),PACK(size,1));
//     }
//     pre_listp = bp;
// }
void *mm_malloc(size_t size)
{
    if(!size)	return NULL;
    //printf("malloc start\n");
    void* blk_ptr = NULL;
    
    // ROUND_UP_FOR EVERY EIGHT BYTE + HEADER_FOOTER_COST
    size = ROUNDUP_EIGHT_BYTES(size) + 8;
    
    // can find fited_block  
    size_t index = sizet(size);
    if((blk_ptr = find(index,size)) != NULL)
    {
        place(blk_ptr,size);
        return blk_ptr;
    }
    
    // extend heap
    blk_ptr = extend_heap(MAX(CHUNKSIZE,size) / WSIZE);
    if(!blk_ptr)	return NULL;
    
    place(blk_ptr,size);
    return blk_ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    //not allowed free
    if(ptr == NULL || ptr <= heap_list || ptr >= mem_heap_hi())	
    	return;
    	
    PUT_HEADER(ptr,GET_SIZE(ptr),0);
    PUT_FOOTER(ptr,GET_SIZE(ptr),0);
    ptr = merge(ptr);
    if(!ptr) 	return;
    size_t size = GET_SIZE(ptr),index = sizet(size);
    insert(blocks_list + index,ptr);
    return;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *newptr;
    
    if(!size)
    {
        mm_free(ptr);
        return NULL;
    }
    
    newptr = mm_malloc(size);
    if(!ptr || !newptr)  return newptr;
    
    size_t copy_size = MIN(size,GET_VALID_SIZE(ptr));
    memcpy(newptr,ptr,copy_size);
    mm_free(ptr);
    return newptr;
}
