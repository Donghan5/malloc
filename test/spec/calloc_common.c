/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   calloc_common.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "calloc_spec.h"

int	has_pattern(unsigned char *data, size_t size, unsigned char value)
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

int	calloc_sample(size_t count, size_t size, int report)
{
	unsigned char	*pointer;
	int				valid;

	pointer = calloc(count, size);
	if (!pointer)
		return (1);
	valid = has_pattern(pointer, count * size, 0);
	if (report)
	{
		ft_putstr_fd("EXPECT calloc(7, 13) = ", 1);
		print_memory_address_portable(pointer);
		ft_putstr_fd("\n", 1);
	}
	memset(pointer, 0x59, count * size);
	valid = valid && has_pattern(pointer, count * size, 0x59);
	free(pointer);
	return (!valid);
}

int	calloc_reuse(void)
{
	unsigned char	*blocks[3];
	uintptr_t		address;
	int				valid;

	blocks[0] = malloc(64);
	blocks[1] = malloc(64);
	blocks[2] = malloc(64);
	if (!blocks[0] || !blocks[1] || !blocks[2])
		return (1);
	memset(blocks[1], 0x93, 64);
	address = (uintptr_t)blocks[1];
	free(blocks[1]);
	blocks[1] = calloc(8, 8);
	if (!blocks[1])
		return (1);
	valid = has_pattern(blocks[1], 64, 0);
	if ((uintptr_t)blocks[1] != address)
		valid = -1;
	free(blocks[0]);
	free(blocks[1]);
	free(blocks[2]);
	if (valid == -1)
		return (77);
	return (!valid);
}

int	calloc_overflow(void)
{
	volatile size_t	count;
	volatile size_t	size;
	t_heap			*anchor;
	void			*pointer;

	count = SIZE_MAX / 2 + 1;
	size = 2;
	anchor = g_data.heap_anchor;
	pointer = calloc(count, size);
	if (pointer || g_data.heap_anchor != anchor)
		return (1);
	count++;
	pointer = calloc(count, size);
	if (pointer || g_data.heap_anchor != anchor)
		return (1);
	return (calloc_sample(7, 13, 0));
}

int	calloc_zero(void)
{
	void	*pointer;
	int		is_null;

	pointer = malloc(0);
	is_null = (pointer == NULL);
	free(pointer);
	pointer = calloc(0, 7);
	if ((pointer == NULL) != is_null)
		return (1);
	free(pointer);
	pointer = calloc(7, 0);
	if ((pointer == NULL) != is_null)
		return (1);
	free(pointer);
	pointer = calloc(0, 0);
	if ((pointer == NULL) != is_null)
		return (1);
	free(pointer);
	return (0);
}
