/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zones.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "zones.h"

int	zone_prepare(t_zone_test *test)
{
	unsigned char	*pointer;
	t_heap			*heap;
	t_block			*block;

	test->original = g_data.heap_anchor;
	while (test->warm_count < ZONE_WARM)
	{
		pointer = malloc(test->maximum);
		if (!pointer)
			return (1);
		search_pointer(&heap, &block, g_data.heap_anchor, pointer);
		if (!heap || !block || heap->group != test->group)
			return (77);
		if (g_data.heap_anchor != test->original && !block->prev)
		{
			test->zone = heap;
			test->live[0] = pointer;
			return (0);
		}
		test->warm[test->warm_count++] = pointer;
	}
	return (77);
}

int	zone_allocate(t_zone_test *test)
{
	size_t	index;
	t_heap	*heap;
	t_block	*block;

	index = 0;
	while (index < ZONE_LIVE)
	{
		if (index)
			test->live[index] = malloc(test->maximum);
		if (!test->live[index])
			return (1);
		search_pointer(&heap, &block, g_data.heap_anchor, test->live[index]);
		if (!heap || !block)
			return (77);
		if (heap == test->zone)
			test->same++;
		memset(test->live[index], index + 1, test->maximum);
		index++;
	}
	return (0);
}

static int	zone_overlap(t_zone_test *test, size_t index)
{
	size_t		other;
	uintptr_t	start;
	uintptr_t	end;

	other = 0;
	start = (uintptr_t)test->live[index];
	end = start + test->maximum;
	if (end < start)
		return (1);
	while (other < index)
	{
		if (start < (uintptr_t)test->live[other] + test->maximum
			&& (uintptr_t)test->live[other] < end)
			return (1);
		other++;
	}
	return (0);
}

int	zone_validate(t_zone_test *test)
{
	size_t	index;
	size_t	byte;

	index = 0;
	while (index < ZONE_LIVE)
	{
		if (zone_overlap(test, index))
			return (1);
		byte = 0;
		while (byte < test->maximum)
		{
			if (test->live[index][byte] != index + 1)
				return (1);
			byte++;
		}
		index++;
	}
	return (0);
}

void	zone_cleanup(t_zone_test *test)
{
	size_t	index;

	index = 0;
	while (index < ZONE_LIVE)
		free(test->live[index++]);
	index = 0;
	while (index < test->warm_count)
		free(test->warm[index++]);
}
