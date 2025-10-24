/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pointer.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/24 15:10:47 by donghank          #+#    #+#             */
/*   Updated: 2025/10/24 15:46:32 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/malloc.h"

/*
** Description: Search pointer
*/
void	search_pointer(t_heap **heap_ptr, t_block **block_ptr, t_heap *heap, void *ptr)
{
	t_block	*block;

	block = NULL;
	while (heap)
	{
		block = (t_block *)HEAP_SHIFT(heap);
		while (block)
		{
			if (BLOCK_SHIFT(block) == ptr)
			{
				*heap_ptr = heap;
				*block_ptr = block;
				return ;
			}
			block = block->next;
		}
		heap = heap->next;
	}
	heap_ptr = NULL;
	block_ptr = NULL;
}