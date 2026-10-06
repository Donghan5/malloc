/* Included only by the M1 variant of edge_cases.c. */
#include <stddef.h>

static int m1_capacity(size_t size)
{
    void *p[100];
    t_heap *first = NULL;
    size_t i;
    for (i = 0; i < 100; ++i)
    {
        p[i] = malloc(size);
        CHECK(p[i] != NULL);
        CHECK((uintptr_t)p[i] % _Alignof(max_align_t) == 0);
        if (i == 0)
            first = owner(p[i]);
        CHECK(owner(p[i]) == first);
        ft_memset(p[i], (int)i, size);
    }
    for (i = 0; i < 100; ++i)
    {
        CHECK(pattern(p[i], size, (unsigned char)i));
        free(p[i]);
    }
    return 0;
}
static int m1_tiny_capacity(void) { return m1_capacity(TINY_BLOCK_SIZE); }
static int m1_small_capacity(void) { return m1_capacity(SMALL_BLOCK_SIZE); }

static int m1_fragmentation(void)
{
    void *p[4096], *q;
    t_heap *first;
    size_t n = 0, i;
    p[0] = malloc(16);
    CHECK(p[0] != NULL);
    first = owner(p[0]);
    ft_memset(p[0], 0x67, 16);
    n = 1;
    while (get_last_block(first)->is_free)
    {
        CHECK(n < 4096);
        p[n] = malloc(16);
        CHECK(p[n] != NULL && owner(p[n]) == first);
        ft_memset(p[n], 0x67, 16);
        ++n;
    }
    /* Exhaust one heap, then make isolated holes of only 16 bytes. */
    for (i = 0; i + 1 < n; i += 2)
        free(p[i]);
    CHECK(first->free_size >= TINY_BLOCK_SIZE + sizeof(t_block));
    q = malloc(TINY_BLOCK_SIZE);
    CHECK(q != NULL && owner(q) != first);
    for (i = 1; i + 1 < n; i += 2)
    {
        CHECK(pattern(p[i], 16, 0x67));
        free(p[i]);
    }
    CHECK(pattern(p[n - 1], 16, 0x67));
    free(p[n - 1]);
    free(q);
    return 0;
}

static int m1_memmove(void)
{
    unsigned char a[64], b[64];
    size_t i;
    for (i = 0; i < 64; ++i) a[i] = (unsigned char)i;
    CHECK(ft_memmove(b, a, 64) == b);
    for (i = 0; i < 64; ++i) CHECK(b[i] == i);
    ft_memmove(a + 7, a, 40);
    for (i = 0; i < 40; ++i) CHECK(a[i + 7] == i);
    ft_memmove(a, a + 7, 40);
    for (i = 0; i < 40; ++i) CHECK(a[i] == i);
    CHECK(ft_memmove(a, a, 64) == a);
    CHECK(ft_memmove(a, b, 0) == a);
    return 0;
}

static int m1_capture(char *output, size_t capacity)
{
    int fd[2], saved;
    ssize_t n;
    CHECK(pipe(fd) == 0);
    saved = dup(1);
    CHECK(saved >= 0 && dup2(fd[1], 1) >= 0);
    close(fd[1]);
    show_alloc_mem();
    CHECK(dup2(saved, 1) >= 0);
    close(saved);
    n = read(fd[0], output, capacity - 1);
    close(fd[0]);
    CHECK(n > 0 && (size_t)n < capacity - 1);
    output[n] = 0;
    return 0;
}

static int m1_output_order(void)
{
    void *p[3];
    t_heap *heaps[3], *tmp;
    char output[4096], *cursor;
    uintptr_t previous = 0, address, total;
    size_t i, j, count = 0;
    const char *digits;
    for (i = 0; i < 3; ++i)
    {
        p[i] = malloc(SMALL_BLOCK_SIZE * 2);
        CHECK(p[i] != NULL);
        heaps[i] = owner(p[i]);
    }
    /* Force an unsorted list regardless of the OS's mmap address policy. */
    for (i = 0; i < 3; ++i)
        for (j = i + 1; j < 3; ++j)
            if ((uintptr_t)heaps[i] < (uintptr_t)heaps[j])
            { tmp = heaps[i]; heaps[i] = heaps[j]; heaps[j] = tmp; }
    for (i = 0; i < 3; ++i)
    {
        heaps[i]->prev = i ? heaps[i - 1] : NULL;
        heaps[i]->next = i < 2 ? heaps[i + 1] : NULL;
    }
    g_data.heap_anchor = heaps[0];
    CHECK(m1_capture(output, sizeof(output)) == 0);
    cursor = output;
    while ((cursor = find_text(cursor, "LARGE : 0x")) != NULL)
    {
        digits = cursor + ft_strlen("LARGE : 0x");
        CHECK(parse_number(&digits, 16, &address));
        CHECK(count == 0 || address > previous);
        previous = address;
        ++count;
        cursor = (char *)digits;
    }
    CHECK(count == 3);
    digits = find_text(output, "Total : ");
    CHECK(digits != NULL);
    digits += ft_strlen("Total : ");
    CHECK(parse_number(&digits, 10, &total));
    CHECK(total == SMALL_BLOCK_SIZE * 6);
    for (i = 0; i < 3; ++i) free(p[i]);
    return 0;
}

static int m1_page_output(void)
{
    char output[4096];
    const char *digits;
    uintptr_t page;
    long expected = sysconf(_SC_PAGESIZE);
    CHECK(expected > 0);
    CHECK(m1_capture(output, sizeof(output)) == 0);
    digits = find_text(output, "Page size : ");
    CHECK(digits != NULL);
    digits += ft_strlen("Page size : ");
    CHECK(parse_number(&digits, 10, &page));
    CHECK(page == (uintptr_t)expected);
    return 0;
}
