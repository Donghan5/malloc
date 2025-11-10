/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   merge_buddy_block.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/22 12:50:20 by donghank          #+#    #+#             */
/*   Updated: 2025/11/10 15:45:58 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../inc/malloc.h"

/*
** Description: Get buddy block of the given block.
*/
static t_block *get_buddy(t_block *block)
{
    uintptr_t buddy_addr = (uintptr_t)block ^ block->data_size;
    
    return ((t_block *)buddy_addr); 
}

/*
** Description: Add block to free list.
*/
void    add_to_free_list(t_block *block, int list_index, t_block **free_lists)
{
    t_block *old_head = free_lists[list_index];

    block->is_free = true;
    block->free_prev = NULL;
    block->free_next = old_head;
    if (old_head != NULL)
        old_head->free_prev = block;
    free_lists[list_index] = block;
}

/*
** Description: merge buddy blocks when freeing a block.
** Modified: return value void-> t_block* (have to tested)
*/
t_block    *free_and_merge_buddies(t_block *block_to_free, t_heap_group group)
{
    t_block **free_lists = (group == TINY) ? g_tiny_free_lists : g_small_free_list;
    block_to_free->is_free = true;
    int order = get_order_from_size(block_to_free->data_size);
    int list_index = order - MIN_ORDER;

    if (order == MAX_ORDER) 
    {
        add_to_free_list(block_to_free, list_index, free_lists);
        return (block_to_free);
    }

    t_block *buddy = get_buddy(block_to_free);

    if (buddy->is_free && buddy->data_size == block_to_free->data_size)
    {
        remove_block_from_free_list(buddy, free_lists);

        t_block *merged_block = (block_to_free < buddy) ? block_to_free : buddy;
        t_block *removed_block = (block_to_free < buddy) ? buddy : block_to_free;
        
        merged_block->data_size *= 2;

        if (removed_block->prev)
            removed_block->prev->next = removed_block->next;
        if (removed_block->next)
            removed_block->next->prev = removed_block->prev;

        return (free_and_merge_buddies(merged_block, group));
    }
	else
    {
        add_to_free_list(block_to_free, list_index, free_lists);
    }
    return (block_to_free);
}