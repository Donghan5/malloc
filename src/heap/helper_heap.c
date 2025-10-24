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

/*
** Description: Determine the heap group based on the requested block size.
*/
size_t   get_heap_group_from_block_size(const size_t size)
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
	if (group == TINY)
		return (TINY_HEAP_ALLOCATION_SIZE);
	else if (group == SMALL)
		return (SMALL_HEAP_ALLOCATION_SIZE);
	return (request_size + sizeof(t_heap) + sizeof(t_block));
}