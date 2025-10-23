/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 19:10:01 by donghank          #+#    #+#             */
/*   Updated: 2025/10/23 14:37:41 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * File: malloc.c
 * Description: Implementation of custom memory allocation functions
 */

#include "../inc/malloc.h"

/*
** Description: Starting malloc function
** Parameters:
**   - size: The size of memory to allocate
** Returns: Pointer to allocated memory or NULL on failure
** Note: 16-byte alignment --> (size + 15) & ~15
*/
void	*start_malloc(size_t size)
{
	t_heap	*heap;
	t_block	*block;
	void	*res;

	if (!size)
		return (NULL);
	size = (size + 15) & ~15;
	if ((block = find_buddy_block(size)) != NULL)
		return (BLOCK_SHIFT(block));
	if (!(heap = get_heap_of_block_size(size)))
		return (NULL);
	res = allocate_block_from_heap(heap, size);
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