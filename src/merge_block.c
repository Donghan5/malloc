/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   merge_block.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/22 12:50:20 by donghank          #+#    #+#             */
/*   Updated: 2025/10/22 13:52:27 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
** File: merge_block.c
** Description: Block Merging Functions
*/

#include "../inc/malloc.h"

/*
** Description: Merge the given block with its previous block if it's free.
** Parameters:
**   heap: The heap containing the blocks.
**   block: The block to merge with its previous block.
** Returns: The merged block (which is the previous block).
*/
static t_block    *merge_prev_block(t_heap *heap, t_block *block)
{
	if (heap && block && block->prev && block->prev->is_free)
	{
		block->prev->next = block->next;
		if (block->next)
			block->next->prev = block->prev;
		t_block *prev_block = block->prev;
		prev_block->data_size += sizeof(t_block) + block->data_size;
		heap->block_count--;
		return (prev_block);
	}
	return (NULL);
}

/*
** Description: Merge the given block with its next block if it's free.
*/
static void    merge_next_block(t_heap *heap,t_block *block)
{
	if (heap && block && block->next && block->next->is_free)
	{
		block->data_size += sizeof(t_block) + block->next->data_size;
		if (block->next && block->next->next)
			block->next->next->prev = block;
		block->next = block->next->next;
		heap->block_count--;
	}
}

/*
** Description: Merge the given block with its adjacent free blocks.
** Parameters:
**   heap: The heap containing the blocks.
**   block: The block to merge.
** Returns: The merged block.
*/
t_block    *merge_block(t_heap *heap, t_block *block)
{
	if (!heap || !block)
		return (NULL);
	block = merge_prev_block(heap, block);
	merge_next_block(heap, block);
	return (block);
}