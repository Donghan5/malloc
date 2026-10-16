/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   calloc_spec.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CALLOC_SPEC_H
# define CALLOC_SPEC_H

# include "malloc.h"
# include <string.h>
# include <sys/wait.h>

int		has_pattern(unsigned char *data, size_t size, unsigned char value);
int		calloc_sample(size_t count, size_t size, int report);
int		calloc_reuse(void);
int		calloc_overflow(void);
int		calloc_failure(void);
int		calloc_zero(void);
int		calloc_threads(int small);

#endif
