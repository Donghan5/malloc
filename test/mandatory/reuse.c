#include "malloc.h"
#include "test_helpers.h"
#include "bonus_helpers.h"
#include "test_ui.h"
int reuse_heap_reuse(void)
{
    size_t i;
    void *p;
    t_heap *first;
    p = malloc(32);
    if (!p) return 1;
    first = owner(p);
    free(p);
    /* Keeping a final empty zone is the mapping-churn contract. Inspect
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
int main(int argc, char **argv) {
 const char *names[] = {"reuse_heap_reuse"};
 int (*cases[])(void) = {reuse_heap_reuse};
 return bonus_run(argc, argv, names, cases, sizeof(cases)/sizeof(cases[0]));
}
