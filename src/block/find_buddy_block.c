/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   find_buddy_block.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/21 13:06:33 by donghank          #+#    #+#             */
/*   Updated: 2025/11/10 11:30:09 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/*
** File: get_block.c
** Description: Block Management Functions
** Using buddy memory allocation system.
*/

#include "../../inc/malloc.h"

/*
** Description: Find order of the given block size.
** Parameters:
**   size: The size to find the order for.
** Returns: The order corresponding to the size.
** Note: block_size is power of 2
*/
int    get_order_from_size(size_t size)
{
    int order;
    size_t block_size;

    order = MIN_ORDER;
    block_size = 1 << order;
    while (block_size < size)
    {
        order++;
        block_size = 1 << order;
    }
    return (order);
}

/*
** Description: Split a big block (if needed)
*/
static t_block    *split_block(t_block *block_to_split, int target_order, t_block **free_lists)
{
    int current_order;

    current_order = get_order_from_size(block_to_split->data_size);
    while (current_order > target_order)
    {
        current_order--;
        size_t new_size = 1 << current_order;

        block_to_split->data_size = new_size;
        
        t_block *buddy_block = (t_block *)((void *)BLOCK_SHIFT(block_to_split) + new_size);
        init_block(buddy_block, new_size);
        buddy_block->is_free = true;
        
        buddy_block->next = block_to_split->next;
        buddy_block->prev = block_to_split;
        if (block_to_split->next)
            block_to_split->next->prev = buddy_block;
        block_to_split->next = buddy_block;

        add_to_free_list(buddy_block, current_order - MIN_ORDER, free_lists);
    }
    return (block_to_split);
}

/*
** Description: Find the buddy block of a given block.
*/
void    find_available_buddy_block(size_t size, t_block **res_block, t_block **free_lists)
{
    int order_needed;
    int list_index;

    order_needed = get_order_from_size(size + sizeof(t_block));
    list_index = order_needed - MIN_ORDER;

    if (free_lists[list_index])
    {
        *res_block = remove_block_from_free_list(free_lists[list_index], free_lists);
        (*res_block)->is_free = false;
        return;
    }

    int larger_list_index;
    larger_list_index = list_index + 1;
    while (larger_list_index < MAX_LISTS)
    {
        if (free_lists[larger_list_index])
        {
            t_block *found = remove_block_from_free_list(free_lists[larger_list_index], free_lists);
            *res_block = split_block(found, order_needed, free_lists);
            (*res_block)->is_free = false;
            return;
        }
        larger_list_index++;
    }
}
