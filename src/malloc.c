/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 19:10:01 by donghank          #+#    #+#             */
/*   Updated: 2025/10/24 14:07:26 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/malloc.h"

/*
** Description: Global variables for memory management
*/
t_heap          *g_heap_anchor = NULL;
t_block         *g_free_lists[MAX_LISTS] = {0};
size_t          g_tiny_heap_count = 0;
size_t          g_small_heap_count = 0;
pthread_mutex_t g_malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
** Description: Starting malloc function
** Parameters:
**   - size: The size of memory to allocate
** Returns: Pointer to allocated memory or NULL on failure
** Note: 16-byte alignment --> (size + 15) & ~15
*/
static void	*start_malloc(size_t size)
{
	t_heap	*heap;
	t_block	*block;
	void	*res;

	if (!size)
		return (NULL);
	block = NULL;
	size = (size + 15) & ~15;
	find_available_buddy_block(size, &block);

	if (block)
		return (BLOCK_SHIFT(block));
	if (!(heap = get_heap_of_block_size(size)))
		return (NULL);
	res = append_empty_block(heap, size);
	return (res);
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