/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 19:32:15 by donghank          #+#    #+#             */
/*   Updated: 2025/10/20 20:01:14 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/**
 * File: realloc.c
 * Description: Implementation of custom realloc function
 */

#include "../include/malloc.h"

/**
 * Function: realloc
 * Description: Reallocates memory block to a new size
 * Parameters:
 *   - ptr: Pointer to the previously allocated memory
 *   - size: New size for the memory block
 */
void	*realloc(void *ptr, size_t size)
{
	(void)ptr;
	(void)size;
	return (NULL);
}