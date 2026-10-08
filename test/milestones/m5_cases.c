/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   m5_cases.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 21:35:37 by donghank          #+#    #+#             */
/*   Updated: 2026/10/08 21:35:37 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include "test_helpers.h"
#include "bonus_helpers.h"
#include "m5_cases.h"
int m5_heap_reuse(void)
{
    size_t i;
    void *p;
    t_heap *first;
    p = malloc(32);
    if (!p) return 1;
    first = owner(p);
    free(p);
    /* Keeping a final empty zone is the M5 mapping-churn contract. Inspect
     * only the current list; never dereference the old unmapped heap. */
    if (g_data.heap_anchor != first) return 1;
    for (i = 0; i < 100; ++i)
    {
        p = malloc(32);
        if (!p || owner(p) != first) return 1;
        free(p);
    }
    return 0;
}
int m5_enabled_regression(void)
{
    volatile size_t overflow = SIZE_MAX;
    unsigned char *p, *q;
    char output[4096];
    int fd, saved;
    if (setenv("MALLOC_DEBUG", "1", 1) || setenv("MALLOC_SCRIBBLE", "1", 1)) return 1;
    fd = capture_begin(&saved); if (fd < 0) return 1;
    p = malloc(64);
    if (!p) return 1;
    ft_memset(p, 0x5a, 64);
    q = realloc(p, SMALL_BLOCK_SIZE + 64);
    if (!q) { free(p); return 1; }
    if (!pattern(q, 64, 0x5a)) return 1;
    p = realloc(q, overflow);
    if (p || !pattern(q, 64, 0x5a)) return 1;
    free(q);
    return capture_end(fd, saved, output, sizeof(output));
}
int main(int argc, char **argv)
{
    const char *names[] = {"M5 final empty heap reuse", "M5 debug/scribble enabled regression"};
    int (*cases[])(void) = {m5_heap_reuse, m5_enabled_regression};
    return bonus_run(argc, argv, names, cases, 2);
}
