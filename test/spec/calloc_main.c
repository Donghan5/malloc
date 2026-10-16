/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   calloc_main.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "calloc_spec.h"

static int	ordinary(void)
{
	if (calloc_sample(1, 1, 0) || calloc_sample(7, 13, 0)
		|| calloc_sample(1, TINY_BLOCK_SIZE, 0)
		|| calloc_sample(1, TINY_BLOCK_SIZE + 1, 0)
		|| calloc_sample(2, SMALL_BLOCK_SIZE / 2, 0)
		|| calloc_sample(1, SMALL_BLOCK_SIZE + 1, 0)
		|| calloc_sample(3, get_page_size(), 0))
		return (1);
	return (0);
}

int	main(int argc, char **argv)
{
	if (argc != 2)
		return (2);
	if (!strcmp(argv[1], "C01"))
		return (ordinary());
	if (!strcmp(argv[1], "C02"))
		return (calloc_reuse());
	if (!strcmp(argv[1], "C03"))
		return (calloc_overflow());
	if (!strcmp(argv[1], "C04"))
		return (calloc_failure());
	if (!strcmp(argv[1], "C05"))
		return (calloc_zero());
	if (!strcmp(argv[1], "C06"))
		return (calloc_sample(7, 13, 1));
	if (!strcmp(argv[1], "C07"))
		return (calloc_threads(0));
	if (!strcmp(argv[1], "C07-debug"))
		return (calloc_threads(1));
	return (2);
}
