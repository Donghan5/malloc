/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   remove_block.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/22 13:50:42 by donghank          #+#    #+#             */
/*   Updated: 2025/10/24 12:18:20 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/malloc.h"

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
        if (block->prev)
            block->prev->next = NULL;
        heap->block_count--;
    }
}

/*
** Description: Remove a block from the free list.
** Parameters:
**   block: The block to remove.
** Returns: None.
*/
t_block   *remove_block_from_free_list(t_block *block)
{
    int order;

    order = get_order_from_size(block->data_size);
    if (block->prev)
        block->prev->next = block->next;
    else
        g_free_lists[order] = block->next;
    if (block->next)
        block->next->prev = block->prev;
    block->prev = NULL;
    block->next = NULL;
    return (block);
}