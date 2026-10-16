/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   calloc_failure.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "calloc_spec.h"

static int	limited_child(void)
{
	struct rlimit	original;
	struct rlimit	limit;
	struct rlimit	actual;
	void			*pointer;

	if (getrlimit(RLIMIT_AS, &original))
		return (77);
	limit = original;
	limit.rlim_cur = 64 * 1024 * 1024;
	if (original.rlim_max < limit.rlim_cur)
		limit.rlim_cur = original.rlim_max;
	if (setrlimit(RLIMIT_AS, &limit) || getrlimit(RLIMIT_AS, &actual)
		|| actual.rlim_cur != limit.rlim_cur)
		return (77);
	pointer = calloc(128 * 1024, 1024);
	if (pointer)
		return (1);
	if (setrlimit(RLIMIT_AS, &original))
		return (77);
	return (calloc_sample(7, 13, 0));
}

int	calloc_failure(void)
{
	pid_t	child;
	int		status;

	child = fork();
	if (child < 0)
		return (77);
	if (child == 0)
		_exit(limited_child());
	if (waitpid(child, &status, 0) != child || !WIFEXITED(status))
		return (1);
	return (WEXITSTATUS(status));
}
