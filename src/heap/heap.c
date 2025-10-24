/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/22 20:49:09 by donghank          #+#    #+#             */
/*   Updated: 2025/10/24 14:07:04 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/malloc.h"

/*
** Description: Get the maximum mmap size allowed by the system.
*/
static rlim_t get_max_mmap_size(void)
{
	struct rlimit rl;

	if (getrlimit(RLIMIT_AS, &rl) == -1)
		return (-1);
	return (rl.rlim_cur);
}

/*
** Description: Initialize a block structure.
*/
void	*append_empty_block(t_heap *heap, size_t size)
{
	t_block	*new_block;
	t_block	*last_block;

	new_block = (t_block *)HEAP_SHIFT(heap);
	last_block = NULL;
	if (heap->block_count)
	{
		last_block = get_last_block(heap);
		new_block = (t_block *)(BLOCK_SHIFT(last_block) + last_block->data_size);
	}
	init_block(new_block, size);
	if (heap->block_count)
	{
		last_block->next = new_block;
		new_block->prev = last_block;
	}
	heap->block_count += 1;
	heap->free_size -= (new_block->data_size + sizeof(t_block));
	return ((void *)BLOCK_SHIFT(new_block));
}

/*
** Description: Create a new heap of the specified group and size.
*/
t_heap    *create_new_heap(const t_heap_group group, const size_t size)
{
    size_t  heap_size;
	t_heap	*heap;

	heap_size = get_heap_group_from_block_size(size);
	if (heap_size > get_max_mmap_size())
		return (NULL);

	heap = (t_heap *)mmap(NULL, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (heap == MAP_FAILED)
		return (NULL);
	ft_memset(heap, 0, heap_size);
	heap->group = group;
	heap->total_size = heap_size;
	heap->free_size = heap_size - sizeof(t_heap);

	if (group == TINY)
		g_tiny_heap_count++;
	else if (group == SMALL)
		g_small_heap_count++;
	
	return (heap);
}

static bool	is_last_of_preallocated(t_heap *heap)
{
	if (heap->group == TINY)
		return (g_tiny_heap_count == 1);
	if (heap->group == SMALL)
		return (g_small_heap_count == 1);

    return (false);
}

void   remove_heap(t_heap *heap)
{
	if (heap->block_count)
		return ;

	if (heap->prev)
		heap->prev->next = heap->next;
	else
		g_heap_anchor = heap->next;
	if (heap->next)
		heap->next->prev = heap->prev;

	if (is_last_of_preallocated(heap))
	{
		if (heap->group == TINY)
			g_tiny_heap_count = 0;
		else if (heap->group == SMALL)
			g_small_heap_count = 0;
	}
	else
	{
		if (heap->group == TINY)
			g_tiny_heap_count--;
		else if (heap->group == SMALL)
			g_small_heap_count--;
	}
	munmap((void *)heap, heap->total_size);
}