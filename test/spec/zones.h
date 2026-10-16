/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zones.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: codex <codex@local>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 00:00:00 by codex             #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by codex            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ZONES_H
# define ZONES_H

# include "malloc.h"
# include <string.h>

# define ZONE_LIVE 100
# define ZONE_WARM 4096

typedef struct s_zone_test
{
	unsigned char	*live[ZONE_LIVE];
	unsigned char	*warm[ZONE_WARM];
	t_heap			*zone;
	t_heap			*original;
	size_t			maximum;
	size_t			page;
	size_t			capacity;
	size_t			zone_size;
	size_t			warm_count;
	size_t			same;
	int				valid;
	int				multiple;
	t_heap_group	group;
}	t_zone_test;

int		zone_prepare(t_zone_test *test);
int		zone_allocate(t_zone_test *test);
int		zone_validate(t_zone_test *test);
void	zone_report(t_zone_test *test);
void	zone_cleanup(t_zone_test *test);

#endif
