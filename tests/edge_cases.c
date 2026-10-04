#include "malloc.h"
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#define CHECK(expr) do { if (!(expr)) { \
    dprintf(2, "  %s:%d: %s\n", __FILE__, __LINE__, #expr); \
    return 1; } } while (0)

static int pattern(const unsigned char *p, size_t n, unsigned char value)
{
    size_t i;
    for (i = 0; i < n; ++i)
        if (p[i] != value)
            return 0;
    return 1;
}

static t_heap *owner(void *p)
{
    t_heap *heap = NULL;
    t_block *block = NULL;
    search_pointer(&heap, &block, g_data.heap_anchor, p);
    return heap;
}

static int zero_and_null(void)
{
    CHECK(malloc(0) == NULL); /* Project's documented zero-size policy. */
    free(NULL);
    CHECK(realloc(NULL, 0) == NULL);
    return 0;
}

static int alignment_and_boundaries(void)
{
    size_t sizes[] = {1, 15, 16, 17, TINY_BLOCK_SIZE - 1,
        TINY_BLOCK_SIZE, TINY_BLOCK_SIZE + 1, SMALL_BLOCK_SIZE - 1,
        SMALL_BLOCK_SIZE, SMALL_BLOCK_SIZE + 1, (size_t)getpagesize(),
        (size_t)getpagesize() + 1};
    void *p[sizeof(sizes) / sizeof(sizes[0])];
    size_t i;
    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
    {
        p[i] = malloc(sizes[i]);
        CHECK(p[i] != NULL && (uintptr_t)p[i] % 16 == 0);
        memset(p[i], (int)(i + 1), sizes[i]);
    }
    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
    {
        CHECK(pattern(p[i], sizes[i], (unsigned char)(i + 1)));
        free(p[i]);
    }
    return 0;
}

static int size_classes(void)
{
    size_t sizes[] = {SMALL_BLOCK_SIZE, TINY_BLOCK_SIZE,
        TINY_BLOCK_SIZE + 1, SMALL_BLOCK_SIZE + 1, 1};
    void *p[5];
    size_t i, aligned;
    for (i = 0; i < 5; ++i)
    {
        p[i] = malloc(sizes[i]);
        CHECK(p[i] != NULL && owner(p[i]) != NULL);
        aligned = (sizes[i] + 15) & ~(size_t)15;
        CHECK(owner(p[i])->group == get_heap_group_from_block_size(aligned));
    }
    for (i = 0; i < 5; ++i)
        free(p[i]);
    return 0;
}

static int overflow(void)
{
    size_t sizes[] = {SIZE_MAX, SIZE_MAX - 14, SIZE_MAX - 15,
        SIZE_MAX - sizeof(t_heap) - sizeof(t_block)};
    size_t i;
    for (i = 0; i < 4; ++i)
        CHECK(malloc(sizes[i]) == NULL);
    CHECK(g_data.heap_anchor == NULL);
    return 0;
}

static int realloc_null_and_zero(void)
{
    unsigned char *p = realloc(NULL, 37);
    CHECK(p != NULL);
    memset(p, 0x71, 37);
    CHECK(realloc(p, 0) == NULL);
    p = malloc(37);
    CHECK(p != NULL);
    free(p);
    return 0;
}

static int realloc_same_and_shrink(void)
{
    unsigned char *p = malloc(128), *q;
    CHECK(p != NULL);
    memset(p, 0x39, 128);
    q = realloc(p, 128);
    CHECK(q == p && pattern(q, 128, 0x39));
    q = realloc(p, 17);
    CHECK(q == p && pattern(q, 17, 0x39));
    free(q);
    return 0;
}

static int realloc_growth(void)
{
    size_t sizes[] = {17, TINY_BLOCK_SIZE, TINY_BLOCK_SIZE + 1,
        SMALL_BLOCK_SIZE + 1, SMALL_BLOCK_SIZE * 8};
    unsigned char *p = malloc(sizes[0]), *q;
    size_t i;
    CHECK(p != NULL);
    memset(p, 0x59, sizes[0]);
    for (i = 1; i < 5; ++i)
    {
        q = realloc(p, sizes[i]);
        CHECK(q != NULL && pattern(q, sizes[i - 1], 0x59));
        p = q;
        memset(p, 0x59, sizes[i]);
    }
    free(p);
    return 0;
}

static int realloc_overflow_preserves_original(void)
{
    unsigned char *p = malloc(64);
    volatile size_t impossible = SIZE_MAX;
    CHECK(p != NULL);
    memset(p, 0xb7, 64);
    CHECK(realloc(p, impossible) == NULL);
    CHECK(pattern(p, 64, 0xb7) && owner(p) != NULL);
    free(p);
    return 0;
}

static int mmap_failure(void)
{
    struct rlimit limit = {0, 0};
    unsigned char *p = malloc(64);
    CHECK(p != NULL);
    memset(p, 0x63, 64);
    CHECK(setrlimit(RLIMIT_AS, &limit) == 0);
    CHECK(malloc(SMALL_BLOCK_SIZE * 16) == NULL);
    CHECK(realloc(p, SMALL_BLOCK_SIZE * 16) == NULL);
    CHECK(pattern(p, 64, 0x63) && owner(p) != NULL);
    free(p);
    return 0;
}

static int large_shrink_lifetime(int reverse)
{
    unsigned char *a = malloc(SMALL_BLOCK_SIZE * 4), *b, *q;
    CHECK(a != NULL);
    memset(a, 0x31, SMALL_BLOCK_SIZE * 4);
    q = realloc(a, SMALL_BLOCK_SIZE * 2);
    CHECK(q != NULL && pattern(q, SMALL_BLOCK_SIZE * 2, 0x31));
    a = q;
    b = malloc(64);
    CHECK(b != NULL);
    memset(b, 0x77, 64);
    if (reverse)
    {
        free(b);
        CHECK(pattern(a, SMALL_BLOCK_SIZE * 2, 0x31));
        free(a);
    }
    else
    {
        free(a);
        CHECK(pattern(b, 64, 0x77));
        free(b);
    }
    return 0;
}

static int large_free_first(void) { return large_shrink_lifetime(0); }
static int small_free_first(void) { return large_shrink_lifetime(1); }

static int split_threshold(void)
{
    _Alignas(16) unsigned char storage[sizeof(t_block) * 2 + 128];
    t_block *block = (t_block *)storage, *tail;
    size_t minimum = sizeof(t_block) + 16;
    init_block(block, 16 + minimum - 16);
    CHECK(split_block(block, 16) == NULL);
    CHECK(block->data_size == minimum && block->next == NULL);
    init_block(block, 16 + minimum);
    tail = split_block(block, 16);
    CHECK(tail != NULL && tail->data_size == 16 && tail->is_free);
    CHECK(tail->prev == block && block->next == tail);
    return 0;
}

static int fragmented_capacity(void)
{
    void *p[400], *q;
    size_t i;
    for (i = 0; i < 400; ++i)
    {
        p[i] = malloc(TINY_BLOCK_SIZE);
        CHECK(p[i] != NULL);
        memset(p[i], (int)(i % 251), TINY_BLOCK_SIZE);
    }
    for (i = 0; i < 400; i += 2)
        free(p[i]);
    q = malloc(SMALL_BLOCK_SIZE);
    CHECK(q != NULL);
    memset(q, 0x93, SMALL_BLOCK_SIZE);
    for (i = 1; i < 400; i += 2)
    {
        CHECK(pattern(p[i], TINY_BLOCK_SIZE, (unsigned char)(i % 251)));
        free(p[i]);
    }
    CHECK(pattern(q, SMALL_BLOCK_SIZE, 0x93));
    free(q);
    return 0;
}

static int large_unlink(void)
{
    void *p[3];
    size_t i;
    for (i = 0; i < 3; ++i)
    {
        p[i] = malloc(SMALL_BLOCK_SIZE * 2);
        CHECK(p[i] != NULL);
        memset(p[i], (int)(i + 1), SMALL_BLOCK_SIZE * 2);
    }
    free(p[1]); /* Middle, then head, then tail. */
    CHECK(pattern(p[0], SMALL_BLOCK_SIZE * 2, 1));
    CHECK(pattern(p[2], SMALL_BLOCK_SIZE * 2, 3));
    free(p[2]);
    CHECK(pattern(p[0], SMALL_BLOCK_SIZE * 2, 1));
    free(p[0]);
    CHECK(g_data.heap_anchor == NULL);
    return 0;
}

static int diagnostics(void)
{
    int fd[2], saved;
    char output[4096];
    ssize_t n;
    void *p = malloc(SMALL_BLOCK_SIZE * 2);
    CHECK(p != NULL && pipe(fd) == 0);
    saved = dup(STDOUT_FILENO);
    CHECK(saved >= 0 && dup2(fd[1], STDOUT_FILENO) >= 0);
    close(fd[1]);
    show_alloc_mem();
    CHECK(dup2(saved, STDOUT_FILENO) >= 0);
    close(saved);
    n = read(fd[0], output, sizeof(output) - 1);
    close(fd[0]);
    CHECK(n > 0);
    output[n] = '\0';
    {
        char expected[64];
        snprintf(expected, sizeof(expected), "Total : %zu bytes", SMALL_BLOCK_SIZE * 2);
        CHECK(strstr(output, "LARGE : ") != NULL);
        CHECK(strstr(output, expected) != NULL);
    }
    free(p);
    return 0;
}

static int diagnostics_address_order(void)
{
    int fd[2], saved;
    char output[4096], *cursor;
    ssize_t n;
    uintptr_t previous = 0;
    size_t count = 0, i;
    void *p[3];
    for (i = 0; i < 3; ++i)
    {
        p[i] = malloc(SMALL_BLOCK_SIZE * (i + 2));
        CHECK(p[i] != NULL);
    }
    CHECK(pipe(fd) == 0);
    saved = dup(1);
    CHECK(saved >= 0 && dup2(fd[1], 1) >= 0);
    close(fd[1]);
    show_alloc_mem();
    CHECK(dup2(saved, 1) >= 0);
    close(saved);
    n = read(fd[0], output, sizeof(output) - 1);
    close(fd[0]);
    CHECK(n > 0);
    output[n] = '\0';
    cursor = output;
    while ((cursor = strstr(cursor, "LARGE : ")) != NULL)
    {
        unsigned long long address;
        CHECK(sscanf(cursor, "LARGE : %llx", &address) == 1);
        CHECK(count == 0 || (uintptr_t)address > previous);
        previous = (uintptr_t)address;
        ++count;
        cursor += 8;
    }
    CHECK(count == 3);
    for (i = 0; i < 3; ++i)
        free(p[i]);
    return 0;
}

int main(void)
{
    struct { const char *name; int (*run)(void); } cases[] = {
        {"zero size / NULL", zero_and_null},
        {"alignment / size boundaries / non-overlap", alignment_and_boundaries},
        {"size class isolation", size_classes},
        {"size and metadata overflow", overflow},
        {"realloc NULL / zero", realloc_null_and_zero},
        {"realloc same size / shrink", realloc_same_and_shrink},
        {"realloc growth across classes", realloc_growth},
        {"realloc overflow preserves original", realloc_overflow_preserves_original},
        {"mmap failure preserves original", mmap_failure},
        {"LARGE shrink: free original first", large_free_first},
        {"LARGE shrink: free neighbor first", small_free_first},
        {"split minimum remainder", split_threshold},
        {"fragmentation / multiple heaps / data survival", fragmented_capacity},
        {"LARGE heap unlink order", large_unlink},
        {"show_alloc_mem total", diagnostics},
        {"show_alloc_mem ascending addresses", diagnostics_address_order}
    };
    size_t i, failures = 0;
    struct rlimit no_core = {0, 0};
    if (setrlimit(RLIMIT_CORE, &no_core) != 0)
    {
        perror("setrlimit RLIMIT_CORE");
        return 1;
    }
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        pid_t pid = fork();
        int status;
        if (pid < 0) { perror("fork"); return 1; }
        if (pid == 0)
        {
            alarm(10);
            _exit(cases[i].run());
        }
        if (waitpid(pid, &status, 0) != pid) { perror("waitpid"); return 1; }
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            printf("PASS %s\n", cases[i].name);
        else
        {
            ++failures;
            printf("FAIL %s", cases[i].name);
            if (WIFSIGNALED(status)) printf(" (signal %d)", WTERMSIG(status));
            putchar('\n');
        }
    }
    printf("%zu/%zu passed\n", i - failures, i);
    puts("UPCOMING FEATURE: thread safety, debug/scribble, show_alloc_mem_ex, defragmentation");
    return failures ? 1 : 0;
}
