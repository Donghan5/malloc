/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   calloc_threads.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "calloc_spec.h"

static pthread_barrier_t	g_start;

static void	*calloc_worker(void *argument)
{
	int				index;
	int				limit;
	unsigned char	*pointer;

	pthread_barrier_wait(&g_start);
	limit = (int)(uintptr_t)argument;
	index = 0;
	while (index < limit)
	{
		pointer = calloc(17, 19);
		if (!pointer || !has_pattern(pointer, 323, 0))
			return ((void *)1);
		memset(pointer, index + 1, 323);
		if (!has_pattern(pointer, 323, index + 1))
			return ((void *)1);
		free(pointer);
		index++;
	}
	return (NULL);
}

static int	start_workers(pthread_t *threads, int small)
{
	int	index;

	if (pthread_barrier_init(&g_start, NULL, 5))
		return (77);
	index = 0;
	while (index < 4)
	{
		if (pthread_create(&threads[index++], NULL, calloc_worker,
				(void *)(uintptr_t)(80 - small * 78)))
			_exit(77);
	}
	g_data.initialized = 0;
	pthread_barrier_wait(&g_start);
	return (0);
}

int	calloc_threads(int small)
{
	pthread_t	threads[4];
	int			index;
	void		*result;
	int			failed;

	if (start_workers(threads, small))
		return (77);
	index = 0;
	failed = 0;
	while (index < 4)
	{
		if (pthread_join(threads[index++], &result) || result)
			failed = 1;
	}
	pthread_barrier_destroy(&g_start);
	return (failed || calloc_overflow() || calloc_failure());
}
