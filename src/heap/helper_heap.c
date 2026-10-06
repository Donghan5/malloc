/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper_heap.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/22 20:59:19 by donghank          #+#    #+#             */
/*   Updated: 2025/10/24 13:25:48 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/malloc.h"
#include <unistd.h>

size_t  get_page_size(void)
{
    long    result;

    result = sysconf(_SC_PAGESIZE);
    if (result <=  0)
        return (0);
    return ((size_t)result);
}

static size_t	round_up_to_page_size(size_t size)
{
	size_t	page_size;

	page_size = get_page_size();
	if (!page_size || size > SIZE_MAX - (page_size - 1))
		return (0);
	return (((size + page_size - 1) / page_size) * page_size);
}

/*
** Description: Determine the heap group based on the requested block size.
*/
t_heap_group   get_heap_group_from_block_size(const size_t size)
{
    if (size <= (size_t)TINY_BLOCK_SIZE)
        return (TINY);
    else if (size <= (size_t)SMALL_BLOCK_SIZE)
        return (SMALL);
    else
        return (LARGE);
}

/*
** Description: Calculate the total heap size needed based on the heap group and requested size.
*/
size_t   get_heap_size_from_block_size(const t_heap_group group, const size_t request_size)
{
	size_t	heap_size;

	if (group == TINY)
		return (TINY_HEAP_ALLOCATION_SIZE);
	else if (group == SMALL)
		return (SMALL_HEAP_ALLOCATION_SIZE);
	if (request_size > SIZE_MAX - sizeof(t_heap))
		return (0);
	heap_size = request_size + sizeof(t_heap);
	if (heap_size > SIZE_MAX - sizeof(t_block))
		return (0);
	return (round_up_to_page_size(heap_size + sizeof(t_block)));
}
