/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 20:32:58 by donghank          #+#    #+#             */
/*   Updated: 2025/10/21 22:13:24 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * File: show_alloc_mem.c
 * Description: Implementation of show_alloc_mem function
 */

#include "../inc/malloc.h"

void    print_heap_header(char *name, t_heap *heap)
{
	ft_putstr_fd(name, 1);
	ft_putstr_fd(" : ", 1);
	print_memory_address_portable((void *)heap);
	ft_putstr_fd("\n", 1);
}

size_t	print_block_list(t_block *block)
{
	t_block *start_addr;
	t_block *end_addr;
	size_t total_size;

	total_size = 0;
	while (block)
	{
		if (!block->is_free)
		{
			start_addr = (t_block *)BLOCK_SHIFT(block);
			end_addr = (t_block *)((char *)start_addr + block->data_size);
			print_memory_address_portable((void *)start_addr);
			ft_putstr_fd(" - ", 1);
			print_memory_address_portable((void *)end_addr);
			ft_putstr_fd(" : ", 1);
			ft_putnbr_fd((int)block->data_size, 1);
			ft_putstr_fd(" octets\n", 1);
			total_size += block->data_size;	
		}
		block = block->next;
	}
	return (total_size);
}

void    print_alloc_mem(void)
{
	t_heap	*first_heap;
	t_heap	*last_heap;
	size_t	total_allocated;

	total_allocated = 0;
	first_heap = g_heap_anchor;
	last_heap = get_last_heap(first_heap);

	while (last_heap)
	{
		if (last_heap->group == TINY)
			print_heap_header("TINY", last_heap);
		else if (last_heap->group == SMALL)
			print_heap_header("SMALL", last_heap);
		else
			print_heap_header("LARGE", last_heap);
		if (last_heap->block_count)
			total_allocated += print_block_list((t_block *)HEAP_SHIFT(last_heap));
		last_heap = last_heap->prev;
	}
	ft_putstr_fd("Total : ", 1);
	ft_print_unsigned_fd(total_allocated, 1);
	ft_putstr_fd(" octets\n", 1);
}

void	show_alloc_mem(void)
{
	pthread_mutex_lock(&g_malloc_mutex);
	print_alloc_mem();
	pthread_mutex_unlock(&g_malloc_mutex);
}