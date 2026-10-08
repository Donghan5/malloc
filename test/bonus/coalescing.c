#include "malloc.h"
#include "test_helpers.h"
#include "bonus_helpers.h"
#include "test_ui.h"
#define CHECK(expr) do { if (!(expr)) { \
    ft_putstr_fd("  " __FILE__ ":", 2); \
    ft_print_unsigned_fd(__LINE__, 2); \
    ft_putstr_fd(": " #expr "\n", 2); \
    return 1; } } while (0)

/* Inspect only mapped metadata, never a payload after free. Bounds and exact
 * physical successors also bound traversal if allocator links form a cycle.
 * LARGE mappings may have unused tail space; TINY/SMALL cover the entire zone.
 * free_size counts free payload AND headers, matching this allocator's policy.
 */
static t_block *block_of(void *p)
{
    return (t_block *)((unsigned char *)p - sizeof(t_block));
}

int coalescing_shrink_regression(void)
{
    unsigned char *a = malloc(128), *b = malloc(64), *c = malloc(64);
    unsigned char *resized;
    t_heap *heap;
    t_block *first;
    size_t merged;

    CHECK(a && b && c);
    heap = owner(a);
    CHECK(heap && owner(b) == heap && owner(c) == heap);
    merged = block_of(a)->data_size + sizeof(t_block) + block_of(b)->data_size;
    ft_memset(a, 0x31, 128);
    ft_memset(c, 0x77, 64);
    CHECK(bonus_valid_state() == 0);
    free(b);
    CHECK(bonus_valid_state() == 0);
    resized = realloc(a, 64);
    CHECK(resized && resized == a);
    a = resized;
    CHECK(pattern(a, 64, 0x31) && pattern(c, 64, 0x77));
    CHECK(bonus_valid_state() == 0); /* Must merge immediately after shrinking. */
    free(a);
    CHECK(pattern(c, 64, 0x77) && bonus_valid_state() == 0);
    first = HEAP_SHIFT(heap);
    CHECK(first->is_free && first->data_size == merged);
    CHECK(first->next == block_of(c) && block_of(c)->prev == first);
    free(c);
    CHECK(bonus_valid_state() == 0);
    return 0;
}

static int shrink_case(size_t size, t_heap_group group, int repeated)
{
    unsigned char *a = malloc(size), *b = malloc(size), *c = malloc(size);
    unsigned char *resized, *reuse;
    t_heap *heap;
    t_block *first;
    size_t target = size / 2;
    size_t expected;

    CHECK(a && b && c);
    heap = owner(a);
    CHECK(heap && heap->group == group && owner(b) == heap && owner(c) == heap);
    expected = block_of(a)->data_size + sizeof(t_block) + block_of(b)->data_size;
    ft_memset(a, 0x23, size);
    ft_memset(c, 0x91, size);
    if (!repeated)
        free(b);
    CHECK(bonus_valid_state() == 0);
    for (;;)
    {
        resized = realloc(a, target);
        CHECK(resized && resized == a);
        a = resized;
        CHECK(pattern(a, target, 0x23) && pattern(c, size, 0x91));
        CHECK(bonus_valid_state() == 0);
        if (!repeated || target <= 64)
            break;
        target /= 2;
    }
    free(a);
    CHECK(bonus_valid_state() == 0);
    if (repeated)
    {
        free(b);
        CHECK(bonus_valid_state() == 0);
    }
    first = HEAP_SHIFT(heap);
    CHECK(first->is_free && first->data_size == expected);
    CHECK(first->next == block_of(c));
    reuse = malloc(size); /* Larger than each piece created by the shrink. */
    CHECK(reuse && owner(reuse) == heap && block_of(reuse) == first);
    CHECK(pattern(c, size, 0x91) && bonus_valid_state() == 0);
    free(reuse);
    free(c);
    CHECK(bonus_valid_state() == 0);
    return 0;
}

int coalescing_tiny_shrink(void)
{
    return shrink_case(TINY_BLOCK_SIZE & ~(size_t)15, TINY, 0);
}

int coalescing_small_shrink(void)
{
    return shrink_case(SMALL_BLOCK_SIZE / 2 & ~(size_t)15, SMALL, 0);
}

int coalescing_repeated_shrink(void)
{
    return shrink_case(SMALL_BLOCK_SIZE & ~(size_t)15, SMALL, 1);
}

/* Live guards isolate three free candidates from the unallocated tail. The
 * larger request can only reuse the hole if removed headers were recovered. */
static int merge_case(int order)
{
    unsigned char *p[5], *reuse;
    t_heap *heap;
    t_block *left;
    size_t size = SMALL_BLOCK_SIZE / 4 & ~(size_t)15;
    size_t expected, i;
    const size_t sequence[3][3] = {{3, 2, 1}, {1, 2, 3}, {1, 3, 2}};

    CHECK(size > TINY_BLOCK_SIZE && 3 * size <= SMALL_BLOCK_SIZE);
    for (i = 0; i < 5; ++i)
    {
        p[i] = malloc(size);
        CHECK(p[i]);
        ft_memset(p[i], (int)(0x40 + i), size);
    }
    heap = owner(p[0]);
    CHECK(heap && heap->group == SMALL);
    for (i = 1; i < 5; ++i)
        CHECK(owner(p[i]) == heap);
    left = block_of(p[1]);
    expected = 3 * size + 2 * sizeof(t_block);
    for (i = 0; i < 3; ++i)
    {
        free(p[sequence[order][i]]);
        CHECK(bonus_valid_state() == 0);
        CHECK(pattern(p[0], size, 0x40) && pattern(p[4], size, 0x44));
    }
    CHECK(left->is_free && left->data_size == expected);
    CHECK(left->prev == block_of(p[0]) && left->next == block_of(p[4]));
    reuse = malloc(3 * size);
    CHECK(reuse && owner(reuse) == heap && block_of(reuse) == left);
    ft_memset(reuse, 0xc5, 3 * size);
    CHECK(pattern(p[0], size, 0x40) && pattern(p[4], size, 0x44));
    CHECK(bonus_valid_state() == 0);
    free(reuse);
    free(p[0]);
    CHECK(pattern(p[4], size, 0x44) && bonus_valid_state() == 0);
    free(p[4]);
    CHECK(bonus_valid_state() == 0);
    return 0;
}

int coalescing_merge_forward(void) { return merge_case(0); }
int coalescing_merge_backward(void) { return merge_case(1); }
int coalescing_merge_both(void) { return merge_case(2); }

int main(int argc, char **argv) {
 const char *names[] = {"coalescing_shrink_regression", "coalescing_tiny_shrink", "coalescing_small_shrink", "coalescing_repeated_shrink", "coalescing_merge_forward", "coalescing_merge_backward", "coalescing_merge_both"};
 int (*cases[])(void) = {coalescing_shrink_regression, coalescing_tiny_shrink, coalescing_small_shrink, coalescing_repeated_shrink, coalescing_merge_forward, coalescing_merge_backward, coalescing_merge_both};
 return bonus_run(argc, argv, names, cases, sizeof(cases)/sizeof(cases[0]));
}
