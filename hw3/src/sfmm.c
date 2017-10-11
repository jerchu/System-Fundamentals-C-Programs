/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"

/**
 * You should store the heads of your free lists in these variables.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
free_list seg_free_list[4] = {
    {NULL, LIST_1_MIN, LIST_1_MAX},
    {NULL, LIST_2_MIN, LIST_2_MAX},
    {NULL, LIST_3_MIN, LIST_3_MAX},
    {NULL, LIST_4_MIN, LIST_4_MAX}
};

int sf_errno = 0;

void *sf_malloc(size_t size) {
    if(size == 0 || size > PAGE_SZ *4)
    {
        //sf_errno = EINVAL
        return NULL;
    }
    if(get_heap_end() - get_heap_start() < PAGE_SZ)
    {
        sf_sbrk();
        get_heap_start();
        for(int i = 3; i >= 0; i--)
        {
            if(seg_free_list[i].min < PAGE_SZ)
            {

                sf_free_header *free_header = get_heap_start();
                //sf_header header = free_header->header;
                free_header->header.allocated = 0;
                free_header->header.padded = 0;
                free_header->header.two_zeroes = 0;
                free_header->header.block_size = 4096 / 16;
                debug("%d", free_header->header.block_size);
                free_header->next = NULL;
                free_header->prev = NULL;
                sf_footer *footer = (get_heap_end() - 8);
                footer->allocated = 0;
                footer->padded = 0;
                footer->two_zeroes = 0;
                footer->block_size = 4096 / 16;
                footer->requested_size = 4096;
                //sf_header *header = free_header->header
                seg_free_list[i].head = free_header;
                debug("%d", i);
                sf_blockprint(seg_free_list[i].head);
                break;
            }
        }
    }
    sf_snapshot();
	return NULL;
}

void *sf_realloc(void *ptr, size_t size) {
	return NULL;
}

void sf_free(void *ptr) {
	return;
}
