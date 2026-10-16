/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   probe.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "probe.h"

static size_t	overflow_size(const char *id)
{
	if (strcmp(id, "O02") == 0)
		return (SIZE_MAX & ~(size_t)15);
	if (strcmp(id, "O03") == 0)
		return ((SIZE_MAX - sizeof(t_heap) - sizeof(t_block)) & ~(size_t)15);
	return (SIZE_MAX);
}

static int	test_overflow(const char *id)
{
	unsigned char	*original;
	void			*result;
	volatile size_t	block_edge;
	int				valid;

	original = malloc(64);
	if (!original)
		return (1);
	memset(original, 0x59, 64);
	if (strcmp(id, "O04") == 0)
		result = realloc(original, overflow_size(id));
	else
		result = malloc(overflow_size(id));
	if (result)
		return (1);
	block_edge = (SIZE_MAX - sizeof(t_heap)) & ~(size_t)15;
	if (strcmp(id, "O02") == 0 && malloc(block_edge))
		return (1);
	valid = probe_pattern(original, 64, 0x59);
	free(original);
	return (!valid);
}

int	main(int argc, char **argv)
{
	long	page;

	if (argc != 2)
		return (2);
	page = sysconf(_SC_PAGESIZE);
	if (page <= 0)
		return (1);
	if (strcmp(argv[1], "E10") == 0)
		return (probe_alignment());
	if (strcmp(argv[1], "O01") == 0 || strcmp(argv[1], "O02") == 0
		|| strcmp(argv[1], "O03") == 0 || strcmp(argv[1], "O04") == 0)
		return (test_overflow(argv[1]));
	return (2);
}
