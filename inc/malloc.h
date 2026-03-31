/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 21:46:29 by donghank          #+#    #+#             */
/*   Updated: 2025/11/10 11:13:50 by donghank         ###   ########.fr       */
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

/// --- @brief global variable for mutex --- ///
extern pthread_mutex_t	g_malloc_mutex;

/// --- @brief wrapper struct for malloc --- ///
extern t_malloc_data    g_data;

#endif
