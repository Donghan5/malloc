/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_block.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/22 13:51:49 by donghank          #+#    #+#             */
/*   Updated: 2025/10/22 21:52:59 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/malloc.h"

void	init_block(t_block *block, const size_t size)
{
	if (block)
	{
		block->data_size = size;
		block->next = NULL;
		block->prev = NULL;
	}
}