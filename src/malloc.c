/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 19:10:01 by donghank          #+#    #+#             */
/*   Updated: 2025/11/10 11:15:30 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/malloc.h"

/*
** Description: Global variables for memory management
*/
t_malloc_data	g_data;
pthread_mutex_t g_malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
** Description: Starting malloc function
** Parameters:
**   - size: The size of memory to allocate
** Returns: Pointer to allocated memory or NULL on failure
** Note: 16-byte alignment --> (size + 15) & ~15
*/
void	*start_malloc(size_t size)
{
	t_heap			*heap;
	t_block			*block;
	t_heap_group	group;

	if (!size)
		return (NULL);
	block = NULL;
	size = (size + 15) & ~15; 
	
	group = get_heap_group_from_block_size(size);

	if (group == LARGE)
	{
		if (!(heap = get_heap_of_block_size(size)))
			return (NULL);
		
		return (append_empty_block(heap, size));
	}
	
	else
	{
		heap = find_free_block(&block, size, g_data.heap_anchor);
		if (block)
		{
			split_block(block, size);
			block->is_free = false;
			heap->free_size -= (block->data_size + sizeof(t_block));
			return(BLOCK_SHIFT(block));
		}
		
		if (!block)
		{
			if (!(heap = get_heap_of_block_size(size)))
				return (NULL);
			block = (t_block *)HEAP_SHIFT(heap);
			if (!block->is_free)
				return (NULL);
			split_block(block, size);
			block->is_free = false;
			heap->free_size -= (block->data_size + sizeof(t_block));
			return (BLOCK_SHIFT(block));
		}
	}

	return (NULL);
}

/**
 * Function: malloc
 * Description: Allocates memory of given size
 * Parameters:
 *   - size: The size of memory to allocate
 */
void	*malloc(size_t size)
{
	void	*result;
	pthread_mutex_lock(&g_malloc_mutex);
	if ((result = start_malloc(size)))
		ft_memset(result, 0xaa, size);
	pthread_mutex_unlock(&g_malloc_mutex);
	return (result);
}