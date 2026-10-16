/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bonus.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "calloc_spec.h"
#include <string.h>

#define THREAD_COUNT 4
#define THREAD_ITERATIONS 80

static int	thread_allocation(unsigned char value, int show)
{
	unsigned char	*original;
	unsigned char	*resized;
	int				valid;

	original = malloc(64);
	if (!original)
		return (1);
	memset(original, value, 64);
	resized = realloc(original, 256);
	if (!resized)
	{
		free(original);
		return (1);
	}
	valid = has_pattern(resized, 64, value);
	if (valid && show)
		show_alloc_mem();
	free(resized);
	return (!valid);
}

static void	*worker(void *argument)
{
	unsigned char	value;
	size_t			index;

	value = (unsigned char)(uintptr_t)argument;
	index = 0;
	while (index < THREAD_ITERATIONS)
	{
		if (thread_allocation(value, index == 0))
			return ((void *)1);
		index++;
	}
	return (NULL);
}

static int	join_threads(pthread_t *threads, size_t count)
{
	size_t	index;
	void	*result;
	int		failed;

	index = 0;
	failed = 0;
	while (index < count)
	{
		result = NULL;
		if (pthread_join(threads[index], &result) != 0 || result != NULL)
			failed = 1;
		index++;
	}
	return (failed);
}

static int	test_concurrency(void)
{
	pthread_t	threads[THREAD_COUNT];
	size_t		index;

	index = 0;
	while (index < THREAD_COUNT)
	{
		if (pthread_create(&threads[index], NULL, worker,
				(void *)(uintptr_t)(index + 1)) != 0)
		{
			join_threads(threads, index);
			return (1);
		}
		index++;
	}
	return (join_threads(threads, THREAD_COUNT));
}

static int	test_debug(void)
{
	unsigned char	*pointer;
	int				valid;

	pointer = malloc(37);
	if (!pointer)
		return (1);
	valid = (g_data.debug == (getenv("MALLOC_DEBUG") != NULL));
	valid = valid && (g_data.scribble == (getenv("MALLOC_SCRIBBLE") != NULL));
	if (g_data.scribble && !has_pattern(pointer, 37, 0xaa))
		valid = 0;
	free(pointer);
	return (!valid);
}

static int	test_hex_dump(void)
{
	unsigned char	*pointer;
	int				valid;

	pointer = malloc(32);
	if (!pointer)
		return (1);
	memset(pointer, 0x12, 32);
	ft_putstr_fd("EXPECT ", 1);
	print_memory_address_portable(pointer);
	ft_putstr_fd(" 32\n", 1);
	show_alloc_mem_ex();
	valid = has_pattern(pointer, 32, 0x12);
	free(pointer);
	return (!valid);
}

static int	prepare_merge_blocks(unsigned char **blocks, size_t size)
{
	size_t	index;
	t_block	*first;
	t_block	*second;

	if (size <= TINY_BLOCK_SIZE || 2 * size > SMALL_BLOCK_SIZE)
		return (1);
	index = 0;
	while (index < 4)
	{
		blocks[index] = malloc(size);
		if (!blocks[index])
			return (1);
		memset(blocks[index], 0x40 + index, size);
		index++;
	}
	first = (t_block *)(blocks[1] - sizeof(t_block));
	second = (t_block *)(blocks[2] - sizeof(t_block));
	if (first->next != second
		|| (unsigned char *)second != blocks[1] + first->data_size)
		return (1);
	return (0);
}

static int	test_coalescing(void)
{
	unsigned char	*blocks[4];
	unsigned char	*reuse;
	size_t			size;
	uintptr_t		address;
	int				valid;

	size = (SMALL_BLOCK_SIZE / 4) & ~(size_t)15;
	if (prepare_merge_blocks(blocks, size))
		return (1);
	address = (uintptr_t)blocks[1];
	free(blocks[1]);
	free(blocks[2]);
	reuse = malloc(2 * size);
	if (!reuse || (uintptr_t)reuse != address)
		return (1);
	memset(reuse, 0x77, 2 * size);
	valid = has_pattern(blocks[0], size, 0x40)
		&& has_pattern(blocks[3], size, 0x43);
	free(reuse);
	free(blocks[0]);
	free(blocks[3]);
	return (!valid);
}

int	main(int argc, char **argv)
{
	if (argc != 2)
		return (2);
	if (strcmp(argv[1], "B01") == 0)
		return (test_concurrency());
	if (strcmp(argv[1], "B02") == 0)
		return (test_debug());
	if (strcmp(argv[1], "B03") == 0)
		return (test_hex_dump());
	if (strcmp(argv[1], "B04") == 0)
		return (test_coalescing());
	return (2);
}
