/*
 * Name: ZiXuan Yuan
 * Student ID: 2200010825
 * An explicit free list implementation of malloc.
 * The free list is segregated, and each free list is sorted by LIFO.
 * Save memory by using offset instead of pointer.
 * Debug the code with mm_checkheap.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memlib.h"
#include "mm.h"

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/*
 * If NEXT_FIT defined use next fit search, else use first-fit search
 */
#define NEXT_FIT

/* Basic constants and macros */
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */
#define MIN_FREE_SIZE 16    /* Minimum free block size (bytes) */
#define FREE_LIST_NUM 27    /* Number of free lists */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc, pred_alloc) ((size) | (alloc) | (pred_alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PRED_ALLOC(p) (GET(p) & 0x2)
#define SET_PRED_ALLOC(p, val) (PUT(p, (GET(p) & ~0X2) | (val)))

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char*)(bp)-WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE(((char*)(bp)-DSIZE)))

/* Given free block ptr bp, compute address of next and previous free blocks */
/* The address is the offset from heap_base, we use it because the offset is in
 * 4 bytes, and then we can save memory. */
/* The address is NULL if the block is the first/last free block */
#define GET_PRED(p) (GET(p) ? heap_base + (GET(p)) : NULL)
#define GET_SUCC(p) \
    (GET((char*)p + WSIZE) ? heap_base + GET((char*)(p) + WSIZE) : NULL)
#define SET_PRED(p, ptr) (PUT((char*)(p), (ptr) ? (char*)(ptr)-heap_base : 0))
#define SET_SUCC(p, ptr) \
    (PUT(((char*)(p) + (WSIZE)), (ptr) ? (char*)(ptr)-heap_base : 0))

/* Global variables */
static char* heap_base = NULL;
static void** free_lists = NULL;

/* Function prototypes for internal helper routines */
static void* extend_heap(size_t words);
static void place(void* bp, size_t asize);
static void* find_fit(size_t asize);
static void* coalesce(void* bp);
static void remove_free(void* bp);
static void push_first_free(void* bp);
static int get_free_index(size_t size);
static void print_block_info(void* ptr);

/*
 * mm_init - Initialize the memory manager
 */
int mm_init(void) {
    heap_base = mem_sbrk((FREE_LIST_NUM + 1) * DSIZE);
    free_lists = (void**)heap_base;
    char* first_bp;
    for (int i = 0; i < FREE_LIST_NUM; i++) {
        free_lists[i] = NULL;
    }
    /* align to 8 bytes */
    first_bp = (char*)(free_lists) + DSIZE * (FREE_LIST_NUM + 1);
    /* Prologue header */
    PUT(HDRP(first_bp), PACK(0, 1, 2));

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * malloc - Allocate a block with at least size bytes of payload
 */
void* malloc(size_t size) {
    // printf("size: %d\n", (int)(size));
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char* bp;
    if (heap_base == NULL) {
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= MIN_FREE_SIZE - WSIZE)
        asize = MIN_FREE_SIZE;
    else
        asize = DSIZE * ((size + WSIZE + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;

    place(bp, asize);
    return bp;
}

/*
 * free - Free a block, and coalesce instantly.
 */
void free(void* bp) {
    if (bp == 0)
        return;
    int pred_alloc = GET_PRED_ALLOC(HDRP(bp));
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0, pred_alloc));
    PUT(FTRP(bp), PACK(size, 0, pred_alloc));

    /* set pred_alloc of next block */
    SET_PRED_ALLOC(HDRP(NEXT_BLKP(bp)), 0);
    coalesce(bp);
    return;
}

/*
 * realloc - Use the realloc from mm-naive.c
 */
void* realloc(void* ptr, size_t size) {
    size_t oldsize;
    void* newptr;
    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL) {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if (!newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if (size < oldsize)
        oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}

/*
 * The remaining routines are internal helper routines
 */

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void* extend_heap(size_t words) {
    char* bp;
    size_t size;
    size_t pred_alloc;
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    pred_alloc = GET_PRED_ALLOC(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0, pred_alloc)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0, pred_alloc)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1, 0));  /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * coalesce - Boundary tag coalescing. Also update free_lists.
 * Return ptr to coalesced block.
 */
static void* coalesce(void* bp) {
    size_t prev_alloc = GET_PRED_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    void* next_bp = NEXT_BLKP(bp);
    void* prev_bp = PREV_BLKP(bp);

    if (prev_alloc && next_alloc) { /* Case 1: both blocks are allocated */
        push_first_free(bp);
        return bp;
    } else if (prev_alloc) { /* Case 2: next block is free */
        size += GET_SIZE(HDRP(next_bp));
        remove_free(next_bp);
        PUT(HDRP(bp), PACK(size, 0, 2));
        PUT(FTRP(bp), PACK(size, 0, 2));
        push_first_free(bp);
        return bp;
    } else if (next_alloc) { /* Case 3: previous block is free */
        size += GET_SIZE(FTRP(prev_bp));
        remove_free(prev_bp);
        PUT(HDRP(prev_bp), PACK(size, 0, GET_PRED_ALLOC(HDRP(prev_bp))));
        PUT(FTRP(prev_bp), PACK(size, 0, GET_PRED_ALLOC(HDRP(prev_bp))));
        push_first_free(prev_bp);
        return prev_bp;
    } else { /* Case 4: both blocks are free */
        size += GET_SIZE(HDRP(prev_bp)) + GET_SIZE(HDRP(next_bp));
        remove_free(prev_bp);
        remove_free(next_bp);
        PUT(HDRP(prev_bp), PACK(size, 0, GET_PRED_ALLOC(HDRP(prev_bp))));
        PUT(FTRP(prev_bp), PACK(size, 0, GET_PRED_ALLOC(HDRP(prev_bp))));
        push_first_free(prev_bp);
        return prev_bp;
    }
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void place(void* bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    void* new_bp;
    remove_free(bp);
    if ((csize - asize) >= MIN_FREE_SIZE) {
        /* split */
        PUT(HDRP(bp), PACK(asize, 1, GET_PRED_ALLOC(HDRP(bp))));
        new_bp = NEXT_BLKP(bp);
        PUT(HDRP(new_bp), PACK(csize - asize, 0, 2));
        PUT(FTRP(new_bp), PACK(csize - asize, 0, 2));
        push_first_free(new_bp);
    } else {
        /* no split */
        PUT(HDRP(bp), PACK(csize, 1, GET_PRED_ALLOC(HDRP(bp))));
        SET_PRED_ALLOC(HDRP(NEXT_BLKP(bp)), 2);
    }
}

/*
 * find_fit - Find a fit for a block with asize bytes
 */
static void* find_fit(size_t asize) {
    int idx = get_free_index(asize);
    void *bp = NULL, *min_bp = NULL;
    size_t min_size = 0xffffffff;
    /* Segregated free list */
    for (; idx < FREE_LIST_NUM; idx++) {
        if (idx < 13) { /* First fit search */
            for (bp = free_lists[idx]; bp != NULL; bp = GET_SUCC(bp)) {
                if (GET_SIZE(HDRP(bp)) >= asize) {
                    return bp;
                }
            }
        } else { /* Best fit search */
            for (bp = free_lists[idx]; bp != NULL; bp = GET_SUCC(bp)) {
                if (GET_SIZE(HDRP(bp)) >= asize) {
                    if (GET_SIZE(HDRP(bp)) < min_size) {
                        min_bp = bp;
                        min_size = GET_SIZE(HDRP(bp));
                    }
                }
            }
            if (min_bp != NULL) {
                return min_bp;
            }
        }
    }
    return NULL; /* No fit */
}

/*
 * remove_free - Remove a free block from free_lists
 */
static void remove_free(void* bp) {
    if (bp == NULL || GET_ALLOC(HDRP(bp))) {
        return;
    }
    int idx = get_free_index(GET_SIZE(HDRP(bp)));
    char* pred_free = GET_PRED(bp);
    char* succ_free = GET_SUCC(bp);

    /* Caution: update free_lists when removing the first free block */
    if (pred_free == NULL)
        free_lists[idx] = succ_free;
    else
        SET_SUCC(pred_free, succ_free);
    if (succ_free != NULL)
        SET_PRED(succ_free, pred_free);
}

/*
 * push_first_free - Push a free block to the first of free_lists
 */
static void push_first_free(void* bp) {
    if (bp == NULL) {
        return;
    }
    int idx = get_free_index(GET_SIZE(HDRP(bp)));
    SET_SUCC(bp, free_lists[idx]);
    SET_PRED(bp, NULL);
    if (free_lists[idx] != NULL) {
        SET_PRED(free_lists[idx], bp);
    }
    free_lists[idx] = bp;
}

/*
 * get_free_index - Get the index of free_lists for a free block
 */
static int get_free_index(size_t size) {
    /* size >= MIN_FREE_SIZE */
    if (size <= 127)
        return (size - MIN_FREE_SIZE) / DSIZE;
    int idx = 6;
    while (size > 0) {
        size >>= 1;
        idx++;
        if (idx >= FREE_LIST_NUM - 1)
            return (FREE_LIST_NUM - 1);
    }
    return idx;
}

/*
 * Print block information for debugging.
 */
static void print_block_info(void* ptr) {
    printf("----------------------------\n");
    printf("### DEBUG Block %p ###\n", ptr);
    printf("Header: 0x%x\n", GET(HDRP(ptr)));
    printf("Footer: 0x%x\n", GET(FTRP(ptr)));
    printf("Size: %d\n", GET_SIZE(HDRP(ptr)));
    printf("Alloc: %d\n", GET_ALLOC(HDRP(ptr)));
    printf("Pred alloc: %d\n", GET_PRED_ALLOC(HDRP(ptr)));
    if (GET_ALLOC(HDRP(ptr)) == 0) {
        printf("Pred: %p\n", GET_PRED(ptr));
        printf("Succ: %p\n", GET_SUCC(ptr));
    }
    printf("---------------------------\n");
}

/*
 * mm_checkheap - Check the heap for correctness. Helpful hint: You
 *                can call this function using mm_checkheap(__LINE__);
 *                to identify the line number of the call site.
 */
void mm_checkheap(int lineno) {
    /* Check base pointer of heap */

    if (heap_base != mem_heap_lo()) {
        printf("Error: Unexpected heap base %p, should be %p\n", heap_base,
               mem_heap_lo());
        exit(1);
    }
    void *ptr, *prev_ptr;
    ptr = (char*)(free_lists) + DSIZE * (FREE_LIST_NUM + 1);
    prev_ptr = NULL;

    while (1) {
        /* Check epilogue block */
        if (GET_SIZE(HDRP(ptr)) == 0 && GET_ALLOC(HDRP(ptr)) == 1) {
            if ((char*)ptr - 1 != (char*)mem_heap_hi()) {
                printf(
                    "Error on block %p: Epilogue block is not at the end "
                    "of "
                    "heap %p\n",
                    ptr, (char*)mem_heap_hi() + 1);
                print_block_info(ptr);
                exit(1);
            }
            break;
        }

        /* Check alignment */
        if ((size_t)ptr % 8) {
            printf("Error on block %p: Not doubleword aligned\n", ptr);
            exit(1);
        }

        /* Check heap boudaries */
        if (ptr < mem_heap_lo() || ptr > mem_heap_hi()) {
            printf(
                "Error on block %p: Out of heap boundaries "
                "(%p, %p)\n",
                ptr, mem_heap_lo(), mem_heap_hi());
            exit(1);
        }

        /* Check header and footer */
        if (GET(HDRP(ptr)) != GET(FTRP(ptr)) && !GET_ALLOC(HDRP(ptr))) {
            printf("Error on block %p: header and footer not match\n", ptr);
            print_block_info(ptr);
            exit(1);
        }

        /* Check minimum block size */
        if (GET_SIZE(HDRP(ptr)) < MIN_FREE_SIZE) {
            printf("Error on block %p: Less than minimum block size %d\n", ptr,
                   MIN_FREE_SIZE);
            print_block_info(ptr);
            exit(1);
        }

        if (prev_ptr != NULL) {
            /* Check prev alloc */
            if (GET_PRED_ALLOC(HDRP(ptr)) != GET_ALLOC(FTRP(prev_ptr)) << 1) {
                printf("Error on block %p: prev alloc not match\n", ptr);
                print_block_info(prev_ptr);
                print_block_info(ptr);
                exit(1);
            }

            /* Check coallescing */
            if (!GET_ALLOC(HDRP(ptr)) && !GET_ALLOC(HDRP(prev_ptr))) {
                printf("Error on block %p: Not coallesced free block\n", ptr);
                print_block_info(prev_ptr);
                print_block_info(ptr);
                exit(1);
            }
        }

        ptr = NEXT_BLKP(ptr);
    }

    for (int i = 0; i < FREE_LIST_NUM; i++) {
        ptr = free_lists[i];
        prev_ptr = NULL;
        while (ptr != NULL) {
            if (prev_ptr != NULL) {
                /* Check if pred/succ pointers are consistent */
                if (prev_ptr != GET_PRED(ptr)) {
                    /* No need to check ptr == GET_SUCC(prev_ptr) */
                    printf("Error on free block %p: prev pointer not match\n",
                           ptr);
                    print_block_info(prev_ptr);
                    print_block_info(ptr);
                    exit(1);
                }
            }

            /* Check heap boundaries */
            if (ptr < mem_heap_lo() || ptr > mem_heap_hi()) {
                printf(
                    "Error on free block %p: Out of heap boundaries "
                    "(%p, %p)\n",
                    ptr, mem_heap_lo(), mem_heap_hi());
                exit(1);
            }

            /* Check size class index */
            if (i != get_free_index(GET_SIZE(HDRP(ptr)))) {
                printf(
                    "Error on free block %p: Wrong size class index %d, "
                    "expected %d",
                    ptr, i, get_free_index(GET_SIZE(HDRP(ptr))));
                exit(1);
            }

            /* Update ptr */
            prev_ptr = ptr;
            ptr = GET_SUCC(ptr);
        }
    }
}
