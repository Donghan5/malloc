/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 20:32:58 by donghank          #+#    #+#             */
/*   Updated: 2025/11/05 17:45:00 by donghank         ###   ########.fr       */
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
	size_t  total_size;

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
			ft_print_unsigned_fd((unsigned long long)block->data_size, 1);
			ft_putstr_fd(" octets\n", 1);
			total_size += block->data_size;	
		}
		block = block->next;
	}
	return (total_size);
}

void    print_alloc_mem(void)
{
	t_heap	*current_heap;
	size_t	total_allocated;

	total_allocated = 0;
	current_heap = g_heap_anchor;
	
	while (current_heap)
	{
		if (current_heap->group == TINY)
			print_heap_header("TINY", current_heap);
		else if (current_heap->group == SMALL)
			print_heap_header("SMALL", current_heap);
		else
			print_heap_header("LARGE", current_heap);
		
		if (current_heap->block_count) 
			total_allocated += print_block_list((t_block *)HEAP_SHIFT(current_heap));
		
        current_heap = current_heap->next;
	}
	ft_putstr_fd("Total : ", 1);
	ft_print_unsigned_fd(total_allocated, 1);
	ft_putstr_fd(" octets\n", 1);
}

void	show_alloc_mem(void)
{
	pthread_mutex_lock(&g_malloc_mutex);
	ft_putstr_fd("===== Show Allocated Memory =====\n", 1);
	print_alloc_mem();
	ft_putstr_fd("=================================\n", 1);
	pthread_mutex_unlock(&g_malloc_mutex);
}