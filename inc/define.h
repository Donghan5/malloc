/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   define.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/22 21:00:53 by donghank          #+#    #+#             */
/*   Updated: 2025/10/23 13:50:16 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DEFINE_H
# define DEFINE_H

/*
** Macros for skipping metadata (heap/block structures)
** when we call malloc, the space is seperated as: metadata + data payload
*/
# define HEAP_SHIFT(start) ((void *)start + sizeof(t_heap))
# define BLOCK_SHIFT(start) ((void *)start + sizeof(t_block))

# define TINY_HEAP_ALLOCATION_SIZE (4 * getpagesize())
# define TINY_BLOCK_SIZE (TINY_HEAP_ALLOCATION_SIZE / 128)
# define SMALL_HEAP_ALLOCATION_SIZE (32 * getpagesize())
# define SMALL_BLOCK_SIZE (SMALL_HEAP_ALLOCATION_SIZE / 128)

/*
** Description: Macros for memory allocation ordering. Minimum block size
** MIN_ORDER: Minimum order (2^4 = 16 bytes)
** MAX_ORDER: Maximum order (2^20 = 1 MB)
** MAX_LISTS: Total number of order lists
*/
#define MIN_ORDER 4
#define MAX_ORDER 20 
#define MAX_LISTS (MAX_ORDER - MIN_ORDER + 1)

#endif
