/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 19:31:42 by donghank          #+#    #+#             */
/*   Updated: 2025/11/10 18:15:23 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * File: free.c
 * Description: Implementation of custom free function
 */
#include "../inc/malloc.h"

void	start_free(void *ptr)
{
	t_heap	*heap;
	t_block	*block;
	t_block	*first;

	heap = g_data.heap_anchor;
	if (!ptr || !heap)
		return ;
	search_pointer(&heap, &block, heap, ptr);
	if (block && heap)
	{
		block->is_free = true;
		
		if (heap->group == TINY || heap->group == SMALL)
		{
			ft_memset(BLOCK_SHIFT(block), 0xdd, block->data_size);
			heap->free_size += (block->data_size + sizeof(t_block));
			if (block->next && block->next->is_free)
				heap->block_count--;
			if (block->prev && block->prev->is_free)
				heap->block_count--;
			coalesce_block(block);
			first = (t_block *)HEAP_SHIFT(heap);
			if (first->prev == NULL && first->next == NULL && first->is_free == true)
			{
				heap->block_count = 0;
				remove_heap(heap);
			}
		}

		else if (heap->group == LARGE)
		{
			ft_memset(BLOCK_SHIFT(block), 0xdd, block->data_size);
			heap->free_size += (block->data_size + sizeof(t_block));
			heap->block_count--;
			remove_heap(heap);
		}
	}
}

/**
 * Function: free
 * Description: Frees the allocated memory pointed by ptr
 * Parameters:
 *   - ptr: Pointer to the memory to be freed
 */

void	free(void *ptr)
{
	pthread_mutex_lock(&g_malloc_mutex);
	start_free(ptr);
	pthread_mutex_unlock(&g_malloc_mutex);
}