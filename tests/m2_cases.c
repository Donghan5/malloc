#include "test_helpers.h"
#include "test_ui.h"
#include "m2_cases.h"

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
static int valid_state(void)
{
    t_heap *heap = g_data.heap_anchor, *previous = NULL;
    size_t tiny = 0, small = 0, heaps = 0;

    while (heap)
    {
        uintptr_t begin = (uintptr_t)heap + sizeof(t_heap);
        uintptr_t end = (uintptr_t)heap + heap->total_size;
        uintptr_t position = begin;
        t_block *block = (t_block *)begin, *prev = NULL;
        size_t count = 0, available = 0;

        CHECK(++heaps <= 4096 && heap->prev == previous);
        CHECK(heap->total_size >= sizeof(t_heap) + sizeof(t_block));
        CHECK(end > begin);
        CHECK(heap->group == TINY || heap->group == SMALL || heap->group == LARGE);
        tiny += heap->group == TINY;
        small += heap->group == SMALL;
        while (block)
        {
            CHECK((uintptr_t)block == position && position <= end);
            CHECK(end - position >= sizeof(t_block));
            CHECK(block->prev == prev);
            CHECK(block->data_size <= end - position - sizeof(t_block));
            CHECK(!prev || !prev->is_free || !block->is_free);
            position += sizeof(t_block) + block->data_size;
            if (block->is_free)
                available += sizeof(t_block) + block->data_size;
            ++count;
            prev = block;
            block = block->next;
        }
        CHECK(count == heap->block_count);
        if (heap->group != LARGE)
            CHECK(position == end);
        else
            available += end - position;
        CHECK(available == heap->free_size);
        previous = heap;
        heap = heap->next;
    }
    CHECK(tiny == g_data.tiny_heap_count);
    CHECK(small == g_data.small_heap_count);
    return 0;
}

static t_block *block_of(void *p)
{
    return (t_block *)((unsigned char *)p - sizeof(t_block));
}

int m2_shrink_regression(void)
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
    CHECK(valid_state() == 0);
    free(b);
    CHECK(valid_state() == 0);
    resized = realloc(a, 64);
    CHECK(resized && resized == a);
    a = resized;
    CHECK(pattern(a, 64, 0x31) && pattern(c, 64, 0x77));
    CHECK(valid_state() == 0); /* Must merge immediately after shrinking. */
    free(a);
    CHECK(pattern(c, 64, 0x77) && valid_state() == 0);
    first = HEAP_SHIFT(heap);
    CHECK(first->is_free && first->data_size == merged);
    CHECK(first->next == block_of(c) && block_of(c)->prev == first);
    free(c);
    CHECK(valid_state() == 0);
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
    CHECK(valid_state() == 0);
    for (;;)
    {
        resized = realloc(a, target);
        CHECK(resized && resized == a);
        a = resized;
        CHECK(pattern(a, target, 0x23) && pattern(c, size, 0x91));
        CHECK(valid_state() == 0);
        if (!repeated || target <= 64)
            break;
        target /= 2;
    }
    free(a);
    CHECK(valid_state() == 0);
    if (repeated)
    {
        free(b);
        CHECK(valid_state() == 0);
    }
    first = HEAP_SHIFT(heap);
    CHECK(first->is_free && first->data_size == expected);
    CHECK(first->next == block_of(c));
    reuse = malloc(size); /* Larger than each piece created by the shrink. */
    CHECK(reuse && owner(reuse) == heap && block_of(reuse) == first);
    CHECK(pattern(c, size, 0x91) && valid_state() == 0);
    free(reuse);
    free(c);
    CHECK(valid_state() == 0);
    return 0;
}

int m2_tiny_shrink(void)
{
    return shrink_case(TINY_BLOCK_SIZE & ~(size_t)15, TINY, 0);
}

int m2_small_shrink(void)
{
    return shrink_case(SMALL_BLOCK_SIZE / 2 & ~(size_t)15, SMALL, 0);
}

int m2_repeated_shrink(void)
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
        CHECK(valid_state() == 0);
        CHECK(pattern(p[0], size, 0x40) && pattern(p[4], size, 0x44));
    }
    CHECK(left->is_free && left->data_size == expected);
    CHECK(left->prev == block_of(p[0]) && left->next == block_of(p[4]));
    reuse = malloc(3 * size);
    CHECK(reuse && owner(reuse) == heap && block_of(reuse) == left);
    ft_memset(reuse, 0xc5, 3 * size);
    CHECK(pattern(p[0], size, 0x40) && pattern(p[4], size, 0x44));
    CHECK(valid_state() == 0);
    free(reuse);
    free(p[0]);
    CHECK(pattern(p[4], size, 0x44) && valid_state() == 0);
    free(p[4]);
    CHECK(valid_state() == 0);
    return 0;
}

int m2_merge_forward(void) { return merge_case(0); }
int m2_merge_backward(void) { return merge_case(1); }
int m2_merge_both(void) { return merge_case(2); }

int m2_heap_boundaries(void)
{
    unsigned char *a = malloc(64), *b = malloc(64), *c = malloc(64);
    t_heap *heap;
    t_block *first, *tail;
    size_t expected;

    CHECK(a && b && c);
    heap = owner(a);
    CHECK(heap && owner(b) == heap && owner(c) == heap);
    first = HEAP_SHIFT(heap);
    tail = block_of(c)->next;
    CHECK(first == block_of(a) && !first->prev);
    CHECK(tail && tail->is_free && !tail->next);
    expected = block_of(c)->data_size + sizeof(t_block) + tail->data_size;
    ft_memset(b, 0x86, 64);
    free(a);
    CHECK(first->is_free && !first->prev && first->next == block_of(b));
    CHECK(valid_state() == 0);
    free(c);
    tail = block_of(b)->next;
    CHECK(tail && tail->is_free && !tail->next && tail->data_size == expected);
    CHECK(tail->prev == block_of(b) && pattern(b, 64, 0x86));
    CHECK(valid_state() == 0);
    free(b);
    CHECK(valid_state() == 0);
    return 0;
}

static int reclaim_group(size_t size, t_heap_group group, int reverse)
{
    unsigned char *p[2048];
    size_t n = 0, i, index;
    size_t *counter = group == TINY ? &g_data.tiny_heap_count : &g_data.small_heap_count;

    while (*counter < 3)
    {
        CHECK(n < sizeof(p) / sizeof(p[0]));
        p[n] = malloc(size);
        CHECK(p[n] && owner(p[n])->group == group);
        ft_memset(p[n], (int)(n % 251 + 1), size);
        ++n;
        CHECK(valid_state() == 0);
    }
    for (i = 0; i < n; ++i)
    {
        index = reverse ? n - i - 1 : i;
        CHECK(pattern(p[index], size, (unsigned char)(index % 251 + 1)));
        free(p[index]);
        CHECK(valid_state() == 0);
        if (i + 1 < n)
        {
            index = reverse ? n - i - 2 : i + 1;
            CHECK(pattern(p[index], size, (unsigned char)(index % 251 + 1)));
        }
    }
    CHECK(*counter <= 1); /* Support retaining the final empty zone or removing it. */
    p[0] = malloc(size);
    CHECK(p[0] && valid_state() == 0);
    free(p[0]);
    CHECK(valid_state() == 0);
    return 0;
}

int m2_heap_reclamation(void)
{
    CHECK(reclaim_group(TINY_BLOCK_SIZE & ~(size_t)15, TINY, 0) == 0);
    CHECK(reclaim_group(TINY_BLOCK_SIZE & ~(size_t)15, TINY, 1) == 0);
    CHECK(reclaim_group(SMALL_BLOCK_SIZE & ~(size_t)15, SMALL, 0) == 0);
    CHECK(reclaim_group(SMALL_BLOCK_SIZE & ~(size_t)15, SMALL, 1) == 0);
    return 0;
}

int main(int argc, char **argv)
{
    const struct { const char *name; int (*run)(void); } cases[] = {
        {"M2 required a/b/c shrink regression", m2_shrink_regression},
        {"M2 TINY shrink / metadata / reuse", m2_tiny_shrink},
        {"M2 SMALL shrink / metadata / reuse", m2_small_shrink},
        {"M2 repeated shrink / consecutive free chain", m2_repeated_shrink},
        {"M2 forward merge / recovered headers / reuse", m2_merge_forward},
        {"M2 backward merge / recovered headers / reuse", m2_merge_backward},
        {"M2 both-side merge / recovered headers / reuse", m2_merge_both},
        {"M2 first and last block boundaries", m2_heap_boundaries},
        {"M2 empty heaps / group counters / reclamation", m2_heap_reclamation}
    };
    size_t count = sizeof(cases) / sizeof(cases[0]);
    uintptr_t index;
    const char *argument;
    int result;

    if (argc == 1)
    {
        ft_print_unsigned_fd(count, 1);
        ft_putstr_fd("\n", 1);
        return 0;
    }
    if (argc != 2)
        return 1;
    argument = argv[1];
    if (!parse_number(&argument, 10, &index) || *argument || index >= count)
        return 1;
    test_ui_start(cases[index].name);
    alarm(10);
    result = cases[index].run();
    test_ui_result(cases[index].name, result == 0);
    return result;
}
