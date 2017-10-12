/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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

// TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO!!!!!!!!!!
// do the page call and coalese!!!!!!!

void *sf_malloc(size_t size) {
    int padding = 0;
    if(size == 0 || size > PAGE_SZ *4)
    {
        sf_errno = EINVAL;
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
                free_header->header.block_size = PAGE_SZ / 16;
                debug("%d", free_header->header.block_size);
                free_header->next = NULL;
                free_header->prev = NULL;
                debug("%p %p %p", free_header, /*(sf_footer *)*/((void *)free_header + (free_header->header.block_size * 16 - 8)), get_heap_end()-8);
                sf_footer *footer = ((void *)free_header + (free_header->header.block_size * 16 - 8));//(get_heap_end() - 8);
                footer->allocated = 0;
                footer->padded = 0;
                footer->two_zeroes = 0;
                footer->block_size = PAGE_SZ / 16;
                footer->requested_size = 4096;
                //sf_header *header = free_header->header
                seg_free_list[i].head = free_header;
                debug("%d", i);
                //sf_blockprint(seg_free_list[i].head);
                debug("%ld", get_heap_end() - get_heap_start());
                break;
            }
        }
    }
    int required_size = size + 32;
    if(required_size % 16)
    {
        required_size += 16 - required_size % 16;
        padding = 1;
    }
    sf_free_header *allocated_block = NULL;
    for(int i = 0; i < 4 && allocated_block == NULL; i++)
    {
        debug("index: %d", i);
        if(seg_free_list[i].max > required_size)
        {
            sf_free_header *current_block = seg_free_list[i].head;
            debug("%d %p", i, current_block);
            while(current_block != NULL)
            {
                if(!(current_block->header.allocated) && current_block->header.block_size >= (required_size + 32)/16 + 1 )
                {
                    if(current_block->prev != NULL){
                        current_block->prev->next = current_block->next;
                    }
                    else
                        seg_free_list[i].head = current_block->next;
                    if(current_block->next != NULL){
                        current_block->next->prev = current_block->prev;
                        current_block->next = NULL;
                    }
                    current_block->prev = NULL;
                    allocated_block = current_block;
                    debug("Doing an insert %p", current_block);
                    int second_block_size = 0;
                    current_block->header.allocated = 1; //now allocated
                    //sf_blockprint(current_block);
                    long *old_footer = ((void *)current_block + (current_block->header.block_size * 16 - 8));
                    *old_footer = 0;
                    //sf_blockprint(current_block);
                    if(current_block->header.block_size - required_size / 16 == 1) //is it splintered? must be 16 to be splintered
                    {
                        required_size += 16;
                        padding = 1;
                    }
                    if(current_block->header.block_size > required_size / 16){
                        second_block_size = current_block->header.block_size - required_size / 16;
                    }
                    current_block->header.block_size = required_size / 16;
                    current_block->header.padded = padding;
                    sf_footer *footer = ((void *)current_block + (current_block->header.block_size * 16 - 8));
                    footer->allocated = 1;
                    footer->padded = padding;
                    footer->block_size = current_block->header.block_size;
                    footer->requested_size = size;
                    sf_free_header *second_block = NULL;
                    if(second_block_size){ //old block was splintered
                        second_block = (sf_free_header *)(footer + 8); //free header of second block
                        /*second_block->next = current_block->next;
                        current_block->next = second_block;
                        second_block->prev = current_block;*/
                        second_block->header.allocated = 0;
                        second_block->header.block_size = second_block_size;
                        sf_footer *second_footer = ((void *)second_block + (second_block->header.block_size * 16 - 8));
                        second_footer->allocated = 0;
                        second_footer->block_size = second_block_size;

                        debug("blah %d", 0);
                    }
                    int listi;
                    for(listi = 0; seg_free_list[listi].max < current_block->header.block_size * 16; listi++);
                    current_block->next = seg_free_list[listi].head;
                    //sf_snapshot();
                    debug("current block next: %p %d", current_block->next, listi);
                    seg_free_list[listi].head = current_block;
                    debug("blah %d", 0);
                    if(second_block != NULL) //second block exists
                    {
                        //sf_snapshot();
                        if(second_block->prev != NULL){
                            second_block->prev->next = second_block->next;
                            second_block->prev = NULL;
                        }
                        else
                            seg_free_list[i].head = second_block->next;
                        if(second_block->next != NULL){
                            second_block->next->prev = second_block->prev;
                            second_block->next = NULL;
                        }
                        for(listi = 0; seg_free_list[listi].max < second_block->header.block_size * 16; listi++);
                        second_block->next = seg_free_list[listi].head;
                        seg_free_list[listi].head = second_block;
                    }

                    //sf_blockprint(current_block);
                }
                else
                {
                    current_block = current_block->next;
                }
            }
        }
    }
    sf_snapshot();
    if(allocated_block == NULL) //TODO: fix for sf_sbrk
    {
        sf_errno = ENOMEM;
    }
	return allocated_block;
}

void *sf_realloc(void *ptr, size_t size) {
	return NULL;
}

void sf_free(void *ptr) {
    sf_free_header *freed_header = ptr;
    int freed_block_size = freed_header->header.block_size;
    freed_header->header.allocated = 0;
    sf_footer *freed_footer = (void *)freed_header + (freed_block_size * 16 - 8);
    freed_footer->allocated = 0;
    sf_free_header *next_coalesce = freed_header->next;
    while(next_coalesce != NULL && !(next_coalesce->header.allocated)){
        freed_header->header.block_size += next_coalesce->header.block_size;
        next_coalesce = next_coalesce->next;
    }
    if(freed_header->prev != NULL){
        freed_header->prev->next = next_coalesce;
    }
    else{
        int i;
        for(i = 0; seg_free_list[i].max < freed_block_size * 16; i++);
        seg_free_list[i].head = next_coalesce;
    }
    if(freed_header->next != NULL){
        freed_header->next->prev = freed_header->prev;
        freed_header->next = NULL;
    }
    freed_header->prev = NULL;
    freed_footer = (void *)freed_header + (freed_header->header.block_size * 16 - 8);
    freed_footer->allocated = 0;
    freed_footer->block_size = freed_header->header.block_size;
    int listi;
    for(listi = 0; seg_free_list[listi].max < freed_header->header.block_size * 16; listi++);
    freed_header->next = seg_free_list[listi].head;
    //sf_snapshot();
    debug("current block next: %p %d", freed_header->next, listi);
    seg_free_list[listi].head = freed_header;
    debug("blah %d", 0);
    return;

}
