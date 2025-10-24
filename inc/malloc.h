/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 21:46:29 by donghank          #+#    #+#             */
/*   Updated: 2025/10/24 13:17:57 by donghank         ###   ########.fr       */
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
# include "functions.h"


extern pthread_mutex_t	g_malloc_mutex;
extern size_t			g_tiny_heap_count;
extern size_t			g_small_heap_count;

extern t_block *g_free_lists[MAX_LISTS];
extern t_heap	*g_heap_anchor;


#endif
