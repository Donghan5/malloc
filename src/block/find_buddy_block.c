/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   find_buddy_block.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/21 13:06:33 by donghank          #+#    #+#             */
/*   Updated: 2025/10/24 13:20:55 by donghank         ###   ########.fr       */
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
    while (block_size < size + sizeof(t_block))
    {
        order++;
        block_size = 1 << order;
    }
    return (order);
}

/*
** Description: Split a big block (if needed)
*/
static t_block    *split_block(t_block *block_to_split, int target_order)
{
    int current_order;

    current_order = get_order_from_size(block_to_split->data_size);
    while (current_order > target_order)
    {
        remove_block_from_free_list(block_to_split);
        current_order--;
        size_t new_size = 1 << current_order;

        block_to_split->data_size = new_size;
        
        t_block *buddy_block = (t_block *)((void *)BLOCK_SHIFT(block_to_split) + new_size);
        init_block(buddy_block, new_size);
        buddy_block->is_free = true;

        add_to_free_list(buddy_block, current_order);
    }
    return (block_to_split);
}

/*
** Description: Find the buddy block of a given block.
*/
void    find_available_buddy_block(size_t size, t_block **res_block)
{
    int order_needed;
    int list_index;

    order_needed = get_order_from_size(size);
    list_index = order_needed - MIN_ORDER;

    if (g_free_lists[list_index])
    {
        *res_block = remove_block_from_free_list(g_free_lists[list_index]);
        (*res_block)->is_free = false;
        return;
    }

    int larger_list_index;
    larger_list_index = list_index + 1;
    while (larger_list_index < MAX_LISTS)
    {
        if (g_free_lists[larger_list_index])
        {
            t_block *found = remove_block_from_free_list(g_free_lists[larger_list_index]);
            *res_block = split_block(found, order_needed);
            (*res_block)->is_free = false;
            return;
        }
        larger_list_index++;
    }
}
