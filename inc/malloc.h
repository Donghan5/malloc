/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 21:46:29 by donghank          #+#    #+#             */
/*   Updated: 2025/10/23 13:50:47 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>
# include <sys/mman.h>
# include <stdlib.h>
# include <stdbool.h>
# include <sys/resource.h>
# include <stdint.h>
# include <pthread.h>
# include "struct.h"
# include "define.h"


extern pthread_mutex_t	g_malloc_mutex;
extern size_t			g_tiny_heap_count;
extern size_t			g_small_heap_count;

extern t_block *g_free_lists[MAX_LISTS];

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
