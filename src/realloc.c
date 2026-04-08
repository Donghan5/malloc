/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 19:32:15 by donghank          #+#    #+#             */
/*   Updated: 2025/11/08 14:34:45 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * File: realloc.c
 * Description: Implementation of custom realloc function
 */

#include "../inc/malloc.h"

void *start_realloc(void *ptr, size_t size)
{
	t_heap			*heap;
	t_block			*block;
	void			*new_ptr;
	size_t			original_size;

	if (!ptr)
		return (start_malloc(size));
	if (size == 0)
	{
		start_free(ptr);
		return (NULL);
	}

	init_debug_flags();
	
	heap = g_data.heap_anchor;
	size = (size + 15) & ~15;

	search_pointer(&heap, &block, heap, ptr);
	if (!heap || !block)
		return (NULL);
	
	original_size = block->data_size;
	if (original_size >= size)
	{
		if (split_block(block, size))
		{
			heap->block_count++;
			heap->free_size += (original_size - size);
			return (ptr);
		}
		return (ptr);
	}

	new_ptr = start_malloc(size);
	if (!new_ptr)
		return (NULL);
	ft_memmove(new_ptr, ptr, original_size);
	start_free(ptr);
	return (new_ptr);
}

static void	debug_realloc(void *ptr, size_t size, void *result)
{
	if (g_data.debug)
	{
		ft_putstr_fd("[MALLOC] ", 1);
		ft_putstr_fd("realloc", 1);
		ft_putstr_fd("(", 1);
		print_memory_address_portable(ptr);
		ft_putstr_fd(", ", 1);
		ft_print_unsigned_fd(size, 1);
		ft_putstr_fd(") = ", 1);
		print_memory_address_portable(result);
		ft_putstr_fd("\n", 1);
	}
}

/**
 * Function: realloc
 * Description: Reallocates memory block to a new size
 * Parameters:
 *   - ptr: Pointer to the previously allocated memory
 *   - size: New size for the memory block
 */
void	*realloc(void *ptr, size_t size)
{
	void *result;
	pthread_mutex_lock(&g_malloc_mutex);
	result = start_realloc(ptr, size);
	debug_realloc(ptr, size, result);
	pthread_mutex_unlock(&g_malloc_mutex);
	return (result);
}