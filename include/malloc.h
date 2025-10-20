/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 21:46:29 by donghank          #+#    #+#             */
/*   Updated: 2025/10/20 19:38:35 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/**
 * File: malloc.h
 * Description: Header file for malloc function
 * function prototype needed
 */

#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

typedef struct s_block
{
    size_t          size;
    struct s_block  *next;
    int             free;
    char            data[1];
}               t_block;

void    *malloc(size_t size);
void    *realloc(void *ptr, size_t size);
void    free(void *ptr);

#endif
