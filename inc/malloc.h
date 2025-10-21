/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 21:46:29 by donghank          #+#    #+#             */
/*   Updated: 2025/10/20 20:55:57 by donghank         ###   ########.fr       */
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
# include <stdbool.h>
# include <sys/resource.h>

# define TINY_HEAP_ALLOCATION_SIZE (4 * getpagesize())
# define TINY_BLOCK_SIZE (TINY_HEAP_ALLOCATION_SIZE / 128)
# define SMALL_HEAP_ALLOCATION_SIZE (32 * getpagesize())
# define SMALL_BLOCK_SIZE (SMALL_HEAP_ALLOCATION_SIZE / 128)

/* 
** Heap and Block structures (about mmaped memory) 
*/
typedef struct s_heap
{
	struct s_heap	*prev;
	struct s_heap	*next;
	size_t			total_size;
	size_t			free_size;
	size_t			block_count;
}					t_heap;

/* 
** Block structure (about user data called by malloc)
*/
typedef struct s_block
{
	struct s_block	*prev;
	struct s_block	*next;
	size_t			data_size;
}					t_block;

/*
** Function prototypes (mandatory)
*/
void	*malloc(size_t size);
void	*realloc(void *ptr, size_t size);
void	free(void *ptr);
void	show_alloc_mem(void);

/*
** Utility function prototypes
*/
void	*ft_memset(void *dest, int value, size_t count);
void	*ft_memmove(void *dest, const void *src, size_t len);
void	ft_putstr_fd(const char *str, int fd);
size_t	ft_strlen(const char *s);
#endif
