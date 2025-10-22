/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   struct.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/22 21:00:02 by donghank          #+#    #+#             */
/*   Updated: 2025/10/22 21:01:10 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STRUCT_H
# define STRUCT_H

#include <stddef.h>

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

#endif