/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   remove_block.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/22 13:50:42 by donghank          #+#    #+#             */
/*   Updated: 2025/11/10 15:15:39 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/malloc.h"
#include <stddef.h>

/*
** Description: Remove a block from the heap, if the block is the last one.
** Parameters:
**   heap: The heap containing the blocks.
**   block: The block to remove.
** Returns: 1 if the block was successfully removed, 0 otherwise.
*/
void    remove_block_if_last(t_heap *heap, t_block *block)
{
    if (heap && block)
    {
        if (block->prev /* && block->next == NULL */)
            block->prev->next = NULL;
        heap->block_count--;
    }
}

// --- find free block to deallocate unused block --- //
t_heap  *find_free_block(t_block **block, size_t size, t_heap *heap_anchor)
{
    t_heap  *heap_curr;
    t_block *block_curr;
    t_heap_group    request_group;

    heap_curr = heap_anchor;
    request_group = get_heap_group_from_block_size(size);
    while (heap_curr)
    {
        if (heap_curr->group != request_group)
        {
            heap_curr = heap_curr->next;
            continue;
        }
        block_curr = HEAP_SHIFT(heap_curr);
        while (block_curr)
        {
            if (block_curr->is_free && block_curr->data_size >= size)
            {
                *block = block_curr;
                return (heap_curr);
            }
            block_curr = block_curr->next;
        }
        heap_curr = heap_curr->next;
    }
    return (NULL);
}

// --- split function --- //
t_block *split_block(t_block *block, size_t size)
{
    if (!block || !size || size >= block->data_size)
        return (NULL);
    if (block->data_size - size < sizeof(t_block) + 16)
        return (NULL);
    
    // --- calculate new block position --- //
    t_block *new_block;
    new_block = BLOCK_SHIFT(block) + size;

    // --- initialize new block --- //
    init_block(new_block, block->data_size - size - sizeof(t_block));
    new_block->is_free = true;
    new_block->prev = block;
    new_block->next = block->next;
    if (block->next)
        block->next->prev = new_block;
    block->next = new_block;
    
    // --- update size of original block (data_size to size) --- //
    block->data_size = size;
    
    return (new_block);
}

// --- coalesce functions --- //
size_t    coalesce_block(t_block *block)
{
    t_block *next_block;
    t_block *prev_block;
    size_t  coalesce_count;

    next_block = block->next;
    prev_block = block->prev;
    coalesce_count = 0;
    
    // --- coalesce with next block ---
    while (next_block && next_block->is_free == true \
        && block->is_free == true)
    {
        block->data_size += next_block->data_size + sizeof(t_block);
        block->next = next_block->next;
        if (block->next)
            block->next->prev = block;
        next_block = block->next;
        coalesce_count += 1;
    }
    // --- coalesce with prev block ---
    while (prev_block && prev_block->is_free == true \
        && block->is_free == true)
    {
        prev_block->data_size += block->data_size + sizeof(t_block);
        prev_block->next = block->next;
        if (prev_block->next)
            prev_block->next->prev = prev_block;
        block = prev_block;
        prev_block = block->prev;
        coalesce_count += 1;
    }
    return (coalesce_count);
}
