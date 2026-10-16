/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   probe_data.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "probe.h"

int	probe_pattern(unsigned char *data, size_t size, unsigned char value)
{
	size_t	index;

	index = 0;
	while (index < size)
	{
		if (data[index] != value)
			return (0);
		index++;
	}
	return (1);
}

int	probe_aligned(void *pointer)
{
	return (pointer != NULL
		&& (uintptr_t)pointer % _Alignof(max_align_t) == 0);
}

size_t	probe_size(size_t index)
{
	if (index == 0)
		return (1);
	if (index < 4)
		return (_Alignof(max_align_t) + index - 2);
	if (index < 7)
		return (TINY_BLOCK_SIZE + index - 5);
	if (index < 10)
		return (SMALL_BLOCK_SIZE + index - 8);
	return (get_page_size() + index - 10);
}
