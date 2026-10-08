#include "malloc.h"
#include "test_helpers.h"
#include "bonus_helpers.h"
#define CHECK(x) do { if (!(x)) { ft_putstr_fd("Assertion: " #x "\n", 2); return 1; } } while (0)
static t_block *block_of(void *p) { return (t_block *)((unsigned char *)p - sizeof(t_block)); }
int zone_heap_boundaries(void)
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
    CHECK(bonus_valid_state() == 0);
    free(c);
    tail = block_of(b)->next;
    CHECK(tail && tail->is_free && !tail->next && tail->data_size == expected);
    CHECK(tail->prev == block_of(b) && pattern(b, 64, 0x86));
    CHECK(bonus_valid_state() == 0);
    free(b);
    CHECK(bonus_valid_state() == 0);
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
        CHECK(bonus_valid_state() == 0);
    }
    for (i = 0; i < n; ++i)
    {
        index = reverse ? n - i - 1 : i;
        CHECK(pattern(p[index], size, (unsigned char)(index % 251 + 1)));
        free(p[index]);
        CHECK(bonus_valid_state() == 0);
        if (i + 1 < n)
        {
            index = reverse ? n - i - 2 : i + 1;
            CHECK(pattern(p[index], size, (unsigned char)(index % 251 + 1)));
        }
    }
    CHECK(*counter <= 1); /* Support retaining the final empty zone or removing it. */
    p[0] = malloc(size);
    CHECK(p[0] && bonus_valid_state() == 0);
    free(p[0]);
    CHECK(bonus_valid_state() == 0);
    return 0;
}

int zone_heap_reclamation(void)
{
    CHECK(reclaim_group(TINY_BLOCK_SIZE & ~(size_t)15, TINY, 0) == 0);
    CHECK(reclaim_group(TINY_BLOCK_SIZE & ~(size_t)15, TINY, 1) == 0);
    CHECK(reclaim_group(SMALL_BLOCK_SIZE & ~(size_t)15, SMALL, 0) == 0);
    CHECK(reclaim_group(SMALL_BLOCK_SIZE & ~(size_t)15, SMALL, 1) == 0);
    return 0;
}


int main(int argc,char **argv) {
 const char *names[]={"zone first/last block metadata", "TINY/SMALL heap reclamation and counters"};
 int (*cases[])(void)={zone_heap_boundaries,zone_heap_reclamation};
 return bonus_run(argc,argv,names,cases,2);
}
