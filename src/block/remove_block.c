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

/*
** Description: Remove a block from the free list.
** Parameters:
**   block: The block to remove.
** Returns: None.
*/
t_block   *remove_block_from_free_list(t_block *block, t_block **free_lists)
{
    int order;
    int list_index;
    
    order = get_order_from_size(block->data_size);
    list_index = order - MIN_ORDER;
    if (block->free_prev)
        block->free_prev->free_next = block->free_next;
    else
        free_lists[list_index] = block->free_next;
    if (block->free_next)
        block->free_next->free_prev = block->free_prev;
    block->free_prev = NULL;
    block->free_next = NULL;
    return (block);
}