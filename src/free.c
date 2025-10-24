/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 19:31:42 by donghank          #+#    #+#             */
/*   Updated: 2025/10/24 15:47:28 by donghank         ###   ########.fr       */
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
	t_block	*ret;

	heap = g_heap_anchor;
	if (!ptr || !heap)
		return ;
	search_pointer(&heap, &block, heap, ptr);
	if (block && heap)
	{
		block->is_free = true;
		ft_memset(block, 0xdd, block->data_size);
		ret = free_and_merge_buddies(block);
		block = ret ? ret : block;
		remove_block_if_last(heap, block);
		remove_heap(heap);
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