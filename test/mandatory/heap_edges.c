#include "malloc.h"
#include "test_helpers.h"
#include "bonus_helpers.h"
#include "test_ui.h"
#define CHECK(x) do { if (!(x)) { ft_putstr_fd("Edge assertion: " #x "\n", 2); return 1; } } while (0)

static int heap_reuse(t_heap_group group, size_t n)
{
    size_t i;
    unsigned char *p = malloc(n);
    t_heap *heap;
    CHECK(p); heap = owner(p); CHECK(heap && heap->group == group);
    free(p);
    CHECK((group == TINY ? g_data.tiny_heap_count : g_data.small_heap_count) == 1);
    for (i = 0; i < 100; ++i)
    {
        p = malloc(n); CHECK(p && owner(p) == heap);
        ft_memset(p, 0x5a, n); CHECK(pattern(p, n, 0x5a));
        free(p); CHECK(bonus_valid_state() == 0);
    }
    return 0;
}
static int tiny_heap_reuse(void) { return heap_reuse(TINY, 32); }
static int small_heap_reuse(void) { return heap_reuse(SMALL, TINY_BLOCK_SIZE + 16); }

int heap_edge_large_reclamation(void)
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

int heap_edge_recovery_after_mapping_failure(void)
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

int main(int argc, char **argv) {
 const char *names[] = {"tiny_heap_reuse", "small_heap_reuse", "heap_edge_large_reclamation", "heap_edge_recovery_after_mapping_failure"};
 int (*cases[])(void) = {tiny_heap_reuse, small_heap_reuse, heap_edge_large_reclamation, heap_edge_recovery_after_mapping_failure};
 return bonus_run(argc, argv, names, cases, sizeof(cases)/sizeof(cases[0]));
}
