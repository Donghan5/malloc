#include "malloc.h"
#include "test_helpers.h"
#include "bonus_helpers.h"
#include "test_ui.h"
#define CHECK(x) do { if (!(x)) { ft_putstr_fd("Edge assertion: " #x "\n", 2); return 1; } } while (0)

int boundary_shrink_with_live_neighbors(void)
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

int boundary_reuse_after_merge(void)
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


int main(int argc, char **argv) {
 const char *names[] = {"boundary_shrink_with_live_neighbors", "boundary_reuse_after_merge"};
 int (*cases[])(void) = {boundary_shrink_with_live_neighbors, boundary_reuse_after_merge};
 return bonus_run(argc, argv, names, cases, sizeof(cases)/sizeof(cases[0]));
}
