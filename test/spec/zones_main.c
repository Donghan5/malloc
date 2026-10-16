/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zones_main.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "zones.h"

static void	field(char *name, size_t value)
{
	ft_putstr_fd(name, 1);
	ft_print_unsigned_fd(value, 1);
}

void	zone_report(t_zone_test *test)
{
	if (test->group == TINY)
		ft_putstr_fd("TINY", 1);
	else
		ft_putstr_fd("SMALL", 1);
	field(" maximum=", test->maximum);
	field(" zone=", test->zone_size);
	field(" page=", test->page);
	field(" capacity=", test->capacity);
	field(" same=", test->same);
	field(" multiple=", test->multiple);
	field(" data=", test->valid);
	field(" warmup=", test->warm_count);
	field(" heap_header=", sizeof(t_heap));
	field(" block_header=", sizeof(t_block));
	field(" alignment=", 16);
	ft_putstr_fd("\n", 1);
}

static int	check_zone(t_zone_test *test, int preload)
{
	int		status;
	void	*previous;

	previous = NULL;
	if (preload)
		previous = malloc(test->maximum);
	status = zone_prepare(test);
	if (!status)
	{
		test->zone_size = test->zone->total_size;
		test->multiple = (test->zone_size % test->page == 0);
		status = zone_allocate(test);
	}
	if (!status)
		test->valid = (zone_validate(test) == 0);
	zone_report(test);
	zone_cleanup(test);
	free(previous);
	if (status)
		return (status);
	return (test->capacity < ZONE_LIVE || test->same != ZONE_LIVE
		|| !test->multiple || !test->valid);
}

int	main(int argc, char **argv)
{
	t_zone_test	test;
	long		page;

	if ((argc != 2 && argc != 3)
		|| (strcmp(argv[1], "TINY") && strcmp(argv[1], "SMALL")))
		return (2);
	memset(&test, 0, sizeof(test));
	page = sysconf(_SC_PAGESIZE);
	if (page <= 0)
		return (77);
	test.page = (size_t)page;
	if (!strcmp(argv[1], "SMALL"))
		test.group = SMALL;
	test.maximum = TINY_BLOCK_SIZE & ~(size_t)15;
	test.zone_size = TINY_HEAP_ALLOCATION_SIZE;
	if (test.group == SMALL)
	{
		test.maximum = SMALL_BLOCK_SIZE & ~(size_t)15;
		test.zone_size = SMALL_HEAP_ALLOCATION_SIZE;
	}
	test.capacity = (test.zone_size - sizeof(t_heap))
		/ (test.maximum + sizeof(t_block));
	return (check_zone(&test, argc == 3));
}
