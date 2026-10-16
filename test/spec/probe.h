/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   probe.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PROBE_H
# define PROBE_H

# include "malloc.h"
# include <stddef.h>
# include <string.h>

# define ALIGNMENT_CASES 12

int		probe_pattern(unsigned char *data, size_t size, unsigned char value);
int		probe_aligned(void *pointer);
size_t	probe_size(size_t index);
int		probe_alignment(void);

#endif
