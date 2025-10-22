/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_block.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/21 13:06:33 by donghank          #+#    #+#             */
/*   Updated: 2025/10/22 12:43:36 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/*
** File: get_block.c
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
** Parameters:
**   size: The size of memory requested.
**   res_heap: Pointer to store the found heap.
**   res_block: Pointer to store the found block.
** Returns: None. The results are stored in res_heap and res_block.
*/
void    find_available_block(size_t size, t_heap **res_heap, t_block **res_block)
{
    t_heap *heap;
    t_block *block;
    heap = g_heap_anchor;

    while (heap)
    {
        if (heap->free_size >= size)
        {
            block = (t_block *)((char *)heap + sizeof(t_heap));
            while (block)
            {
                if (block->data_size >= size)
                {
                    *res_heap = heap;
                    *res_block = block;
                    return ;
                }
                block = block->next;
            }
        }
        heap = heap->next;
    }
}