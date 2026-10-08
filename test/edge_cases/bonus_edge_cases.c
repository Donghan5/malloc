/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bonus_edge_cases.c                                 :+:      :+:    :+:   */
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
#include "bonus_edge_cases.h"

#define CHECK(x) do { if (!(x)) { ft_putstr_fd("Edge assertion: " #x "\n", 2); return 1; } } while (0)

int edge_m2_split_boundary(void)
{
    /* Synthetic mapped block: test exactly below/at the minimum remainder
     * without relying on free-list selection or reading freed memory. */
    size_t length = get_page_size();
    void *mapping = mmap(NULL, length, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    t_block *block = mapping, *tail;
    CHECK(mapping != MAP_FAILED);
    init_block(block, 64 + sizeof(t_block));
    CHECK(split_block(block, 64) == NULL);
    CHECK(block->data_size == 64 + sizeof(t_block) && !block->next);
    init_block(block, 64 + sizeof(t_block) + 16);
    tail = split_block(block, 64);
    CHECK(tail && tail->data_size == 16 && tail->is_free);
    CHECK(block->next == tail && tail->prev == block && !tail->next);
    CHECK(munmap(mapping, length) == 0);
    return 0;
}

int edge_m2_shrink_with_live_neighbors(void)
{
    unsigned char *left = malloc(32), *middle = malloc(96), *right = malloc(32);
    unsigned char *q;
    CHECK(left && middle && right);
    ft_memset(left, 0x11, 32); ft_memset(middle, 0x22, 96); ft_memset(right, 0x33, 32);
    q = realloc(middle, 16);
    CHECK(q && pattern(q, 16, 0x22));
    CHECK(pattern(left, 32, 0x11) && pattern(right, 32, 0x33));
    CHECK(bonus_valid_state() == 0);
    free(q); free(right); free(left);
    return bonus_valid_state();
}

int edge_m2_reuse_after_merge(void)
{
    unsigned char *a = malloc(32), *b = malloc(32), *guard = malloc(32), *q;
    uintptr_t address = (uintptr_t)a;
    CHECK(a && b && guard);
    ft_memset(guard, 0x5a, 32);
    free(a); free(b);
    CHECK(bonus_valid_state() == 0);
    q = malloc(64);
    CHECK(q && (uintptr_t)q == address && pattern(guard, 32, 0x5a));
    free(q); free(guard);
    return bonus_valid_state();
}

int edge_m3_runtime_scribble_toggle(void)
{
    unsigned char *p;
    CHECK(unsetenv("MALLOC_SCRIBBLE") == 0);
    p = malloc(16); CHECK(p); ft_memset(p, 0x5a, 16); free(p);
    CHECK(setenv("MALLOC_SCRIBBLE", "1", 1) == 0);
    p = malloc(16); CHECK(p && pattern(p, 16, 0xaa)); free(p);
    CHECK(setenv("MALLOC_SCRIBBLE", "0", 1) == 0);
    p = malloc(16); CHECK(p && g_data.scribble == 0);
    ft_memset(p, 0x5a, 16); free(p);
    CHECK(setenv("MALLOC_SCRIBBLE", "", 1) == 0);
    p = malloc(1); CHECK(p && pattern(p, 1, 0xaa)); free(p);
    return 0;
}

int edge_m3_realloc_null_scribble(void)
{
    unsigned char *p;
    CHECK(setenv("MALLOC_SCRIBBLE", "1", 1) == 0);
    CHECK(realloc(NULL, 0) == NULL);
    p = realloc(NULL, 1);
    CHECK(p && pattern(p, 1, 0xaa));
    free(p);
    return 0;
}

int edge_m3_freed_dump_exclusion(void)
{
    unsigned char *p = malloc(32), *guard = malloc(32);
    char output[8192];
    int saved, fd;
    CHECK(p && guard);
    ft_memset(p, 0xab, 32); ft_memset(guard, 0x12, 32);
    free(p);
    fd = capture_begin(&saved); CHECK(fd >= 0);
    show_alloc_mem_ex();
    CHECK(capture_end(fd, saved, output, sizeof(output)) == 0);
    CHECK(!find_text(output, "ab ab ab ab"));
    CHECK(find_text(output, "12 12 12 12"));
    CHECK(find_text(output, "Total : 32 bytes\n"));
    CHECK(pattern(guard, 32, 0x12)); free(guard);
    return 0;
}

int edge_m4_early_return_unlock(void)
{
    unsigned char *p;
    CHECK(malloc(0) == NULL);
    free(NULL);
    CHECK(realloc(NULL, 0) == NULL);
    p = malloc(16); CHECK(p);
    ft_memset(p, 0x5a, 16);
    CHECK(realloc(p, 0) == NULL);
    p = malloc(16); CHECK(p); free(p);
    show_alloc_mem(); show_alloc_mem_ex();
    return bonus_valid_state();
}

static pthread_mutex_t first_use_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t first_use_ready = PTHREAD_COND_INITIALIZER;
static int first_use_open;

static void *first_use_worker(void *argument)
{
    size_t n = (size_t)(uintptr_t)argument;
    unsigned char *p;
    int failed;
    pthread_mutex_lock(&first_use_lock);
    while (!first_use_open) pthread_cond_wait(&first_use_ready, &first_use_lock);
    pthread_mutex_unlock(&first_use_lock);
    p = malloc(n);
    if (!p) return (void *)1;
    failed = !pattern(p, n, 0xaa);
    free(p);
    return (void *)(uintptr_t)failed;
}

int edge_m4_concurrent_first_use(void)
{
    pthread_t threads[8];
    size_t created = 0, i;
    int failed = 0;
    CHECK(setenv("MALLOC_SCRIBBLE", "1", 1) == 0);
    for (i = 0; i < 8; ++i)
    {
        if (pthread_create(&threads[i], NULL, first_use_worker, (void *)(uintptr_t)(i + 1)))
        { failed = 1; break; }
        ++created;
    }
    /* Open the gate even after a partial create failure, so created workers
     * are never stranded waiting for a fixed-size barrier. */
    pthread_mutex_lock(&first_use_lock);
    first_use_open = 1;
    pthread_cond_broadcast(&first_use_ready);
    pthread_mutex_unlock(&first_use_lock);
    for (i = 0; i < created; ++i)
    {
        void *result = NULL;
        if (pthread_join(threads[i], &result) || result) failed = 1;
    }
    CHECK(!failed && g_data.initialized == 1);
    return bonus_valid_state();
}

int edge_m4_realloc_failure_unlock(void)
{
    volatile size_t overflow = SIZE_MAX;
    unsigned char *p = malloc(32), *q;
    CHECK(p); ft_memset(p, 0x5a, 32);
    CHECK(realloc(p, overflow) == NULL && pattern(p, 32, 0x5a));
    CHECK(malloc(overflow) == NULL);
    q = malloc(16); CHECK(q); free(q); free(p);
    return bonus_valid_state();
}

int edge_m5_small_heap_reuse(void)
{
    size_t i, n = TINY_BLOCK_SIZE + 16;
    unsigned char *p = malloc(n);
    t_heap *heap;
    CHECK(p); heap = owner(p); CHECK(heap && heap->group == SMALL);
    free(p);
    CHECK(g_data.small_heap_count == 1);
    for (i = 0; i < 100; ++i)
    {
        p = malloc(n); CHECK(p && owner(p) == heap);
        ft_memset(p, 0x5a, n); CHECK(pattern(p, n, 0x5a));
        free(p); CHECK(bonus_valid_state() == 0);
    }
    return 0;
}

int edge_m5_large_reclamation(void)
{
    size_t i, n = SMALL_BLOCK_SIZE + 16;
    unsigned char *a = malloc(n), *b = malloc(n), *c = malloc(n);
    t_heap *heap;
    CHECK(a && b && c);
    ft_memset(a, 0x11, n); ft_memset(c, 0x33, n);
    free(b); CHECK(pattern(a, n, 0x11) && pattern(c, n, 0x33));
    free(a); CHECK(pattern(c, n, 0x33)); free(c);
    for (i = 0, heap = g_data.heap_anchor; heap; heap = heap->next)
    { CHECK(++i <= 4096 && heap->group != LARGE); }
    return bonus_valid_state();
}

int edge_m5_recovery_after_mapping_failure(void)
{
    struct rlimit original, restricted;
    unsigned char *p = malloc(32), *q;
    CHECK(p); ft_memset(p, 0x5a, 32);
    CHECK(getrlimit(RLIMIT_AS, &original) == 0);
    restricted = original; restricted.rlim_cur = 0;
    CHECK(setrlimit(RLIMIT_AS, &restricted) == 0);
    q = realloc(p, SMALL_BLOCK_SIZE + 16);
    CHECK(setrlimit(RLIMIT_AS, &original) == 0);
    CHECK(q == NULL && pattern(p, 32, 0x5a));
    q = realloc(p, SMALL_BLOCK_SIZE + 16);
    CHECK(q && pattern(q, 32, 0x5a)); free(q);
    return bonus_valid_state();
}
