/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_block.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/21 13:06:33 by donghank          #+#    #+#             */
/*   Updated: 2025/10/21 13:06:34 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/*
** File: block.c
** Description: Block Management Functions
** Using buddy memory allocation system.
*/

#include "../inc/malloc.h"

/*
** Description: Get the last block in a linked list of blocks.
*/
t_block    *get_last_block(t_block *block)
{
    while (block && block->next)
        block = block->next;
    return (block);
}

/*
** Description: Find an available block in the heap.
*/
void    find_available_block(size_t size, t_heap **res_heap, t_block **res_block)
{
    t_heap *heap;
    t_block *block;
}