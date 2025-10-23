/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   merge_buddy_block.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/22 12:50:20 by donghank          #+#    #+#             */
/*   Updated: 2025/10/23 14:06:19 by donghank         ###   ########.fr       */
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
** Description: merge buddy blocks when freeing a block.
*/
void    free_and_merge_buddies(t_block *block_to_free)
{
    block_to_free->is_free = true;
    int order = get_order_from_size(block_to_free->data_size);

    if (order == MAX_ORDER) 
    {
        add_to_free_list(block_to_free, order);
        return;
    }

    t_block *buddy = get_buddy(block_to_free);

    if (buddy->is_free && buddy->data_size == block_to_free->data_size)
    {
        remove_from_free_list(buddy);
        
        t_block *merged_block = (block_to_free < buddy) ? block_to_free : buddy;
        merged_block->data_size *= 2;
        
        free_and_merge_buddies(merged_block); 
    }
    {
        add_to_free_list(block_to_free, order);
    }
}