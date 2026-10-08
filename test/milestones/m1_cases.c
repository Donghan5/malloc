/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   m1_cases.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 21:35:37 by donghank          #+#    #+#             */
/*   Updated: 2026/10/08 21:35:37 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "m1_cases.h"
#include "malloc.h"
#include "test_helpers.h"
#include <stddef.h>

#define CHECK(expr) do { if (!(expr)) { \
    ft_putstr_fd("  " __FILE__ ":", 2); \
    ft_print_unsigned_fd(__LINE__, 2); \
    ft_putstr_fd(": " #expr "\n", 2); \
    return 1; } } while (0)


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
int m1_tiny_capacity(void) { return m1_capacity(TINY_BLOCK_SIZE); }
int m1_small_capacity(void) { return m1_capacity(SMALL_BLOCK_SIZE); }

int m1_fragmentation(void)
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

int m1_memmove(void)
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

static int m1_capture_function(char *output, size_t capacity, void (*display)(void))
{
    char path[] = "/tmp/malloc-output-XXXXXX";
    int fd, saved;
    ssize_t n;
    fd = mkstemp(path);
    CHECK(fd >= 0);
    CHECK(unlink(path) == 0);
    saved = dup(1);
    CHECK(saved >= 0 && dup2(fd, 1) >= 0);
    display();
    CHECK(dup2(saved, 1) >= 0);
    close(saved);
    CHECK(lseek(fd, 0, SEEK_SET) == 0);
    n = read(fd, output, capacity - 1);
    close(fd);
    CHECK(n >= 0 && (size_t)n < capacity - 1);
    output[n] = 0;
    ft_putstr_fd(output, 1);
    return 0;
}

static int m1_capture(char *output, size_t capacity)
{
    return m1_capture_function(output, capacity, show_alloc_mem);
}

int m1_output_order(void)
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

int m1_page_output(void)
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

/* Compare every printed allocation with the live payload and block metadata. */
static int m1_verify_output(void **p, size_t count)
{
    char output[8192], *line, *next;
    const char *digits;
    uintptr_t address, end, bytes, previous_heap = 0, previous_block = 0;
    uintptr_t expected_total = 0, printed_total = 0;
    t_heap *heap;
    t_block *block;
    size_t i, live = 0, headers = 0, expected_headers = 0, printed = 0;
    unsigned int seen = 0;
    int total_seen = 0;
    for (heap = g_data.heap_anchor; heap; heap = heap->next) ++expected_headers;
    for (i = 0; i < count; ++i)
        if (p[i])
        {
            search_pointer(&heap, &block, g_data.heap_anchor, p[i]);
            CHECK(heap && block && !block->is_free);
            expected_total += block->data_size;
            ++live;
        }
    CHECK(m1_capture(output, sizeof(output)) == 0);
    for (line = output; *line; line = next)
    {
        next = line;
        while (*next && *next != '\n') ++next;
        if (*next) *next++ = 0;
        if (starts_with(line, "TINY : ") || starts_with(line, "SMALL : ")
            || starts_with(line, "LARGE : "))
        {
            digits = find_text(line, "0x");
            CHECK(digits != NULL);
            digits += 2;
            CHECK(parse_number(&digits, 16, &address) && !*digits);
            CHECK(headers == 0 || address > previous_heap);
            for (heap = g_data.heap_anchor; heap && (uintptr_t)heap != address; heap = heap->next) {}
            CHECK(heap != NULL);
            CHECK(starts_with(line, heap->group == TINY ? "TINY : " :
                heap->group == SMALL ? "SMALL : " : "LARGE : "));
            previous_heap = address;
            ++headers;
        }
        else if (starts_with(line, "0x"))
        {
            digits = line + 2;
            CHECK(parse_number(&digits, 16, &address) && starts_with(digits, " - 0x"));
            digits += 5;
            CHECK(parse_number(&digits, 16, &end) && starts_with(digits, " : "));
            digits += 3;
            CHECK(parse_number(&digits, 10, &bytes) && starts_with(digits, " bytes"));
            CHECK(printed == 0 || address > previous_block);
            for (i = 0; i < count && (uintptr_t)p[i] != address; ++i) {}
            CHECK(i < count && !(seen & (1U << i)));
            seen |= 1U << i;
            search_pointer(&heap, &block, g_data.heap_anchor, p[i]);
            CHECK(heap && block && !block->is_free);
            CHECK((uintptr_t)heap == previous_heap);
            CHECK(bytes == block->data_size && end == address + bytes);
            printed_total += bytes;
            previous_block = address;
            ++printed;
        }
        else if (starts_with(line, "Total : "))
        {
            digits = line + ft_strlen("Total : ");
            CHECK(parse_number(&digits, 10, &bytes));
            CHECK(bytes == expected_total && bytes == printed_total);
            ++total_seen;
        }
    }
    CHECK(total_seen == 1 && printed == live && headers == expected_headers);
    return 0;
}

int m1_mixed_output(void)
{
    void *p[7];
    size_t sizes[] = {17, 48, 64, TINY_BLOCK_SIZE + 1, SMALL_BLOCK_SIZE,
        SMALL_BLOCK_SIZE + 1, SMALL_BLOCK_SIZE * 3};
    t_heap *heaps[16], *heap, *tmp;
    size_t i, j, n = 0;
    for (i = 0; i < 7; ++i) { p[i] = malloc(sizes[i]); CHECK(p[i] != NULL); }
    /* Include all classes and multiple blocks, with an OS-independent list order. */
    for (heap = g_data.heap_anchor; heap; heap = heap->next)
    { CHECK(n < 16); heaps[n++] = heap; }
    for (i = 0; i < n; ++i)
        for (j = i + 1; j < n; ++j)
            if ((uintptr_t)heaps[i] < (uintptr_t)heaps[j])
            { tmp = heaps[i]; heaps[i] = heaps[j]; heaps[j] = tmp; }
    for (i = 0; i < n; ++i)
    {
        heaps[i]->prev = i ? heaps[i - 1] : NULL;
        heaps[i]->next = i + 1 < n ? heaps[i + 1] : NULL;
    }
    g_data.heap_anchor = heaps[0];
    CHECK(m1_verify_output(p, 7) == 0);
    free(p[1]); p[1] = NULL; /* Free holes must not be printed or added to Total. */
    free(p[5]); p[5] = NULL; /* Verify output after a heap is unlinked. */
    CHECK(m1_verify_output(p, 7) == 0);
    for (i = 0; i < 7; ++i) free(p[i]);
    for (i = 0; i < 7; ++i) p[i] = NULL;
    CHECK(m1_verify_output(p, 7) == 0);
    return 0;
}

int m1_empty_output(void)
{
    CHECK(g_data.heap_anchor == NULL);
    return m1_verify_output(NULL, 0);
}

int m1_page_geometry(void)
{
    size_t page = get_page_size(), i, aligned, expected;
    size_t sizes[] = {1, TINY_BLOCK_SIZE, TINY_BLOCK_SIZE + 1,
        SMALL_BLOCK_SIZE, SMALL_BLOCK_SIZE + 1, page - 1, page, page + 1};
    void *p;
    t_heap *heap;
    char output[4096];
    const char *digits;
    uintptr_t printed;
    CHECK(page > 0 && page == (size_t)sysconf(_SC_PAGESIZE));
    CHECK(TINY_HEAP_ALLOCATION_SIZE == page * 4);
    CHECK(SMALL_HEAP_ALLOCATION_SIZE == page * 32);
    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
    {
        p = malloc(sizes[i]);
        CHECK(p != NULL);
        heap = owner(p);
        CHECK(heap && (uintptr_t)heap % page == 0 && heap->total_size % page == 0);
        aligned = (sizes[i] + 15) & ~(size_t)15;
        if (heap->group == LARGE)
        {
            expected = aligned + sizeof(t_heap) + sizeof(t_block);
            expected = ((expected + page - 1) / page) * page;
        }
        else expected = heap->group == TINY ? page * 4 : page * 32;
        CHECK(heap->total_size == expected);
        free(p);
    }
    CHECK(get_heap_size_from_block_size(LARGE, SIZE_MAX) == 0);
    CHECK(get_heap_size_from_block_size(LARGE, SIZE_MAX - sizeof(t_heap) - sizeof(t_block)) == 0);
    CHECK(m1_capture_function(output, sizeof(output), show_alloc_mem_ex) == 0);
    digits = find_text(output, "Page size : ");
    CHECK(digits != NULL);
    digits += ft_strlen("Page size : ");
    CHECK(parse_number(&digits, 10, &printed) && printed == page);
    return 0;
}
