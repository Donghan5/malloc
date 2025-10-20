/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 21:46:29 by donghank          #+#    #+#             */
/*   Updated: 2025/10/20 20:53:38 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/**
 * File: malloc.h
 * Description: Header file for malloc function
 * function prototype needed
 */

#ifndef MALLOC_H
# define MALLOC_H

# include <stddef.h>
# include <unistd.h>
# include <sys/mman.h>
# include <stdlib.h>

# define TINY_HEAP_ALLOCATION_SIZE (4 * getpagesize())
# define TINY_BLOCK_SIZE (TINY_HEAP_ALLOCATION_SIZE / 128)
# define SMALL_HEAP_ALLOCATION_SIZE (32 * getpagesize())
# define SMALL_BLOCK_SIZE (SMALL_HEAP_ALLOCATION_SIZE / 128)

typedef struct s_block
{
	struct s_block	*prev;
	struct s_block	*next;
	size_t			total_size;
	size_t			free_size;
	size_t			block_count;
}					t_block;

void	*malloc(size_t size);
void	*realloc(void *ptr, size_t size);
void	free(void *ptr);
void	show_alloc_mem(void);

#endif
