/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 21:46:29 by donghank          #+#    #+#             */
/*   Updated: 2025/10/21 22:04:31 by donghank         ###   ########.fr       */
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
# include <stdint.h>
# include <pthread.h>

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

extern pthread_mutex_t	g_malloc_mutex;
extern t_heap			*g_heap_anchor;

/*
** Heap group enumeration
** - For pagesize of 4096 bytes
** TINY - block < 128 bytes - heap 16 KB
** SMALL - block < 1024 bytes - heap 128 KB
** LARGE - block >= 1024 bytes
*/
typedef enum e_heap_group
{
	TINY,
	SMALL,
	LARGE
}				t_heap_group;


/* 
** Heap and Block structures (about mmaped memory) 
*/
typedef struct s_heap
{
	struct s_heap	*prev;
	struct s_heap	*next;
	t_heap_group	group;
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
	bool			is_free;
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
