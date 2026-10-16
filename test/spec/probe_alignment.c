/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   probe_alignment.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "probe.h"

/* Keep every block live to exercise payload offsets after metadata. */
static int	allocate_alignment_blocks(unsigned char **blocks)
{
	size_t	index;
	size_t	size;

	index = 0;
	while (index < ALIGNMENT_CASES)
	{
		size = probe_size(index);
		blocks[index] = malloc(size);
		if (!probe_aligned(blocks[index]))
			return (1);
		memset(blocks[index], 0x35, size);
		index++;
	}
	return (0);
}

static int	resize_block(unsigned char **block, size_t kept, size_t requested)
{
	unsigned char	*resized;

	resized = realloc(*block, requested);
	if (!resized)
		return (1);
	*block = resized;
	if (!probe_aligned(resized) || !probe_pattern(resized, kept, 0x35))
		return (1);
	return (0);
}

static int	resize_alignment_blocks(unsigned char **blocks)
{
	size_t	index;
	size_t	size;
	size_t	grown;

	index = 0;
	while (index < ALIGNMENT_CASES)
	{
		size = probe_size(index);
		grown = size + SMALL_BLOCK_SIZE + _Alignof(max_align_t);
		if (resize_block(&blocks[index], size, grown)
			|| resize_block(&blocks[index], 1, 1))
			return (1);
		free(blocks[index]);
		index++;
	}
	return (0);
}

int	probe_alignment(void)
{
	unsigned char	*blocks[ALIGNMENT_CASES];
	unsigned char	*pointer;

	if (allocate_alignment_blocks(blocks) || resize_alignment_blocks(blocks))
		return (1);
	pointer = realloc(NULL, _Alignof(max_align_t) + 1);
	if (!probe_aligned(pointer))
		return (1);
	free(pointer);
	ft_putstr_fd("Linux ABI alignment=", 1);
	ft_print_unsigned_fd(_Alignof(max_align_t), 1);
	ft_putstr_fd(" bytes; live size cases=", 1);
	ft_print_unsigned_fd(ALIGNMENT_CASES, 1);
	ft_putstr_fd("\n", 1);
	return (0);
}
