/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bonus_helpers.c                                    :+:      :+:    :+:   */
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

int capture_begin(int *saved)
{
    char path[] = "/tmp/malloc-bonus-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return -1;
    unlink(path);
    *saved = dup(1);
    if (*saved < 0 || dup2(fd, 1) < 0)
    { if (*saved >= 0) close(*saved); close(fd); return -1; }
    return fd;
}

int capture_end(int fd, int saved, char *output, size_t capacity)
{
    ssize_t n;
    int restored = dup2(saved, 1);
    close(saved);
    if (restored < 0 || lseek(fd, 0, SEEK_SET) < 0)
    { close(fd); return 1; }
    n = read(fd, output, capacity - 1);
    close(fd);
    if (n < 0 || (size_t)n == capacity - 1) return 1;
    output[n] = 0;
    ft_putstr_fd(output, 1);
    return 0;
}

int bonus_run(int argc, char **argv, const char *const *names,
    int (*const *cases)(void), size_t count)
{
    uintptr_t index;
    const char *arg;
    if (argc == 1) { ft_print_unsigned_fd(count, 1); ft_putstr_fd("\n", 1); return 0; }
    if (argc != 2) return 1;
    arg = argv[1];
    if (!parse_number(&arg, 10, &index) || *arg || index >= count) return 1;
    ft_putstr_fd(names[index], 1); ft_putstr_fd("\n", 1);
    alarm(10);
    return cases[index]();
}

#define CHECK(x) do { if (!(x)) { ft_putstr_fd("State assertion: " #x "\n", 2); return 1; } } while (0)
int bonus_valid_state(void)
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
