/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    if(size == 0 || size > PAGE_SZ * 4)
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
    int required_size = size + 16;
    if(required_size % 16)
    {
        required_size += 16 - required_size % 16;
        padding = 1;
        debug("req_size padding: %d", padding);
    }
    sf_free_header *allocated_block = NULL;
    for(int i = 0; i < 4 && allocated_block == NULL; i++)
    {
        //debug("index: %d", i);
        if(seg_free_list[i].max > required_size)
        {
            sf_free_header *current_block = seg_free_list[i].head;
            //debug("%d %p", i, current_block);
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
                    //sf_snapshot();
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
                        debug("splinter padding: %d", padding);
                    }
                    if(current_block->header.block_size > required_size / 16){
                        second_block_size = current_block->header.block_size - required_size / 16;
                    }
                    current_block->header.block_size = required_size / 16;
                    debug("%d are we padding?: %d", required_size, padding);
                    current_block->header.padded = padding;
                    sf_footer *footer = ((void *)current_block + (current_block->header.block_size * 16 - 8));
                    footer->allocated = 1;
                    footer->padded = padding;
                    footer->block_size = current_block->header.block_size;
                    footer->requested_size = size;
                    sf_free_header *second_block = NULL;
                    if(second_block_size){ //old block was splintered
                        second_block = (void *)current_block + current_block->header.block_size * 16; //free header of second block
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
                    /*int listi;
                    for(listi = 0; seg_free_list[listi].max < current_block->header.block_size * 16; listi++);
                    current_block->next = seg_free_list[listi].head;
                    if(seg_free_list[listi].head)
                        seg_free_list[listi].head->prev = current_block;
                    //sf_snapshot();
                    debug("current block next: %p %d", current_block->next, listi);
                    seg_free_list[listi].head = current_block;
                    //sf_snapshot();
                    debug("blah %d", 0);*/
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
                        int listi;
                        for(listi = 0; seg_free_list[listi].max < second_block->header.block_size * 16; listi++);
                        second_block->next = seg_free_list[listi].head;
                        seg_free_list[listi].head = second_block;
                    }
                    debug("current_block padded: %d", current_block->header.padded);

                    //sf_blockprint(current_block);
                }
                else
                {
                    current_block = current_block->next;
                }
            }
        }
    }
    //sf_snapshot();
    if(allocated_block == NULL) //TODO: fix for sf_sbrk
    {
        sf_errno = ENOMEM;
    }
    debug("allocated_block padded: %d", allocated_block->header.padded);
	return allocated_block;
}

void *sf_realloc(void *ptr, size_t size) {
    if(!size){
        sf_free(ptr);
        return NULL;
    }
    sf_free_header *new_block = NULL;
    sf_free_header *realloc_header = ptr;
    debug("%p %p", ptr, get_heap_start());
    if(ptr < get_heap_start() || ptr > get_heap_end() || ptr == NULL)
        abort();
    if(!(realloc_header->header.allocated))
        abort();
    sf_footer *realloc_footer = (void *)realloc_header + (realloc_header->header.block_size * 16 - 8);
    debug("%p %p", realloc_footer, get_heap_end());
    if((void *)realloc_footer < get_heap_start() || (void *)realloc_footer > get_heap_end())
        abort();
    if(!(realloc_footer->allocated) || realloc_footer->padded != realloc_header->header.padded)
        abort();
    if(realloc_footer->requested_size + 16 != realloc_header->header.block_size && !(realloc_header->header.padded))
        abort();
    if(realloc_header->header.block_size < size){
        new_block = sf_malloc(size);
        if(!new_block)
            return NULL;
        memcpy(new_block, realloc_header, 8);
        memcpy((void *)new_block+8, (void *)realloc_header+8, realloc_footer->requested_size);
        memcpy((void *)new_block + (new_block->header.block_size * 16 - 8), realloc_footer, 8);
        sf_free(realloc_header);
    }
    else if(realloc_header->header.block_size > size){
        //int block_size = realloc_header->header.block_size;
        int second_block_size = 0;
        int padding = 0;
        sf_free_header *second_block = NULL;
        int required_size = size + 16;
        if(required_size % 16)
        {
            required_size += 16 - required_size % 16;
            padding = 1;
        }
        if(realloc_header->header.block_size - required_size / 16 == 1) //is it splintered? must be 16 to be splintered
        {
            required_size += 16;
            padding = 1;
        }
        if(realloc_header->header.block_size > required_size / 16){
            second_block_size = realloc_header->header.block_size - required_size / 16;
        }
        realloc_header->header.block_size = required_size / 16;
        realloc_header->header.padded = padding;
        memcpy((void *)realloc_header+8, (void *)realloc_header+8, size);
        realloc_footer = (void *)realloc_header + required_size - 8;
        realloc_footer->allocated = 1;
        realloc_footer->padded = padding;
        realloc_footer->two_zeroes = 0;
        realloc_footer->block_size = realloc_header->header.block_size;
        realloc_footer->requested_size = size;
        if(second_block_size){
            second_block = (void *)realloc_header + realloc_header->header.block_size * 16; //free header of second block
            /*second_block->next = current_block->next;
            current_block->next = second_block;
            second_block->prev = current_block;*/
            second_block->header.allocated = 1;
            second_block->header.block_size = second_block_size;
            second_block->header.padded = 0;
            sf_footer *second_footer = ((void *)second_block + (second_block->header.block_size * 16 - 8));
            second_footer->allocated = 1;
            second_footer->block_size = second_block_size;
            second_footer->padded = 0;
            second_footer->requested_size = second_block_size - 16;
            sf_free(second_block);
        }
        /*if(second_block){
            int listi;
            for(listi = 0; seg_free_list[listi].max < second_block->header.block_size * 16; listi++);
            second_block->next = seg_free_list[listi].head;
            seg_free_list[listi].head = second_block;
        }*/
        new_block = realloc_header;
    }
    else{
        new_block = realloc_header;
    }
	return new_block;
}

void sf_free(void *ptr) {
    sf_free_header *freed_header = ptr;
    debug("%p %p", ptr, get_heap_start());
    if(ptr < get_heap_start() || ptr > get_heap_end() || ptr == NULL)
        abort();
    if(!(freed_header->header.allocated))
        abort();
    int freed_block_size = freed_header->header.block_size;
    freed_header->header.allocated = 0;
    sf_footer *freed_footer = (void *)freed_header + (freed_block_size * 16 - 8);
    debug("%p %p", freed_footer, get_heap_end());
    if((void *)freed_footer < get_heap_start() || (void *)freed_footer > get_heap_end())
        abort();
    if(!(freed_footer->allocated) || freed_footer->padded != freed_header->header.padded)
        abort();
    debug("%d %d", freed_footer->requested_size + 16 != freed_header->header.block_size * 16, !(freed_header->header.padded));
    if(freed_footer->requested_size + 16 != freed_header->header.block_size * 16 && !(freed_header->header.padded))
        abort();
    freed_footer->allocated = 0;
    sf_free_header *next_coalesce = (void *)freed_header + freed_header->header.block_size * 16;
    //sf_blockprint(next_coalesce);
    while((void *)next_coalesce < get_heap_end() && !(next_coalesce->header.allocated)){
        //debug("end head: %p", get_heap_end());
        //debug("next_coalesce: %p", next_coalesce);
        freed_header->header.block_size += next_coalesce->header.block_size;
        if(next_coalesce->prev != NULL){
            next_coalesce->prev->next = next_coalesce->next;
        }
        else{
            int i;
            for(i = 0; seg_free_list[i].max < next_coalesce->header.block_size * 16; i++);
            seg_free_list[i].head = next_coalesce->next;
            if(seg_free_list[i].head != NULL)
                seg_free_list[i].head->prev = NULL;
        }
        if(next_coalesce->next != NULL){
            next_coalesce->next->prev = next_coalesce->prev;
        }
        next_coalesce = (void *)next_coalesce + next_coalesce->header.block_size * 16;
    }
    //sf_snapshot();
    /*if(freed_header->prev != NULL){
        freed_header->prev->next = freed_header->next;
    }
    else{
        int i;
        for(i = 0; seg_free_list[i].max < freed_block_size * 16; i++);
        seg_free_list[i].head = freed_header->next;
    }
    if(freed_header->next != NULL){
        freed_header->next->prev = freed_header->prev;

    }*/
    //sf_snapshot();
    freed_header->next = NULL;
    freed_header->prev = NULL;
    //sf_blockprint(freed_header);
    freed_footer = (void *)freed_header + (freed_header->header.block_size * 16 - 8);
    freed_footer->allocated = 0;
    freed_footer->block_size = freed_header->header.block_size;
    freed_footer->requested_size = 0;
    int listi;
    for(listi = 0; seg_free_list[listi].max < freed_header->header.block_size * 16; listi++);
    debug("%p", seg_free_list[listi].head);
    freed_header->next = seg_free_list[listi].head;
    //sf_snapshot();
    debug("current block next: %p %d", freed_header->next, listi);
    if(seg_free_list[listi].head)
        seg_free_list[listi].head->prev = freed_header;
    seg_free_list[listi].head = freed_header;
    debug("blah %d", 0);
    //sf_snapshot();
    return;

}
