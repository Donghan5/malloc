#include "test_helpers.h"
#include "bonus_helpers.h"
#include "m4_cases.h"

/* Each worker owns its payload. This mutex also excludes dump reads from
 * payload writes; allocator entry points retain their own internal mutex. */
static pthread_mutex_t payload_lock = PTHREAD_MUTEX_INITIALIZER;
static int injected;
static size_t detected;
static void *worker(void *argument)
{
    size_t i, id = (size_t)(uintptr_t)argument;
    unsigned char *p, *q, value = (unsigned char)(id + 1);
    for (i = 0; i < 120; ++i)
    {
        size_t n = i % 3 == 0 ? 32 : (i % 3 == 1 ? TINY_BLOCK_SIZE + 16 : SMALL_BLOCK_SIZE + 32);
        pthread_mutex_lock(&payload_lock);
        p = malloc(n);
        if (!p) { pthread_mutex_unlock(&payload_lock); return (void *)1; }
        ft_memset(p, value, n);
        if (injected) p[0] ^= 1;
        if (!pattern(p, n, value))
        { free(p); pthread_mutex_unlock(&payload_lock); return (void *)(uintptr_t)(injected ? 2 : 1); }
        pthread_mutex_unlock(&payload_lock);
        /* Concurrent public calls on independent payloads. */
        q = realloc(p, n + 64);
        if (!q) { free(p); return (void *)1; }
        if (!pattern(q, n, value)) { free(q); return (void *)1; }
        p = realloc(q, 16);
        if (!p) { free(q); return (void *)1; }
        if (!pattern(p, 16, value)) { free(p); return (void *)1; }
        free(p);
        pthread_mutex_lock(&payload_lock);
        if (i % 40 == 0) { show_alloc_mem(); show_alloc_mem_ex(); }
        pthread_mutex_unlock(&payload_lock);
    }
    return NULL;
}

static int stress(size_t count)
{
    pthread_t threads[8];
    size_t created = 0, i;
    int failed = 0, fd, saved;
    char output[262144];
    fd = capture_begin(&saved);
    if (fd < 0) return 1;
    for (i = 0; i < count; ++i)
    {
        if (pthread_create(&threads[i], NULL, worker, (void *)(uintptr_t)i))
        { failed = 1; break; }
        ++created;
    }
    for (i = 0; i < created; ++i)
    {
        void *result = NULL;
        if (pthread_join(threads[i], &result) || result) failed = 1;
        if (result == (void *)2) ++detected;
    }
    if (capture_end(fd, saved, output, sizeof(output))) failed = 1;
    if (bonus_valid_state()) failed = 1;
    return failed;
}
int m4_stress(void)
{
    return stress(2) || stress(4) || stress(8);
}
int m4_failure_detection(void)
{
    int result;
    detected = 0;
    injected = 1;
    result = stress(4);
    injected = 0;
    return result == 1 && detected == 4 ? 0 : 1;
}

static void *release_received(void *arg)
{
    unsigned char *p = arg;
    int failed = !pattern(p, 64, 0x5a);
    free(p);
    return (void *)(uintptr_t)failed;
}
int m4_handoff(void)
{
    pthread_t thread;
    void *result;
    unsigned char *p = malloc(64);
    if (!p) return 1;
    ft_memset(p, 0x5a, 64);
    /* pthread_create publishes completed writes; parent relinquishes ownership. */
    if (pthread_create(&thread, NULL, release_received, p)) { free(p); return 1; }
    if (pthread_join(thread, &result)) return 1;
    return result != NULL || bonus_valid_state();
}
int main(int argc, char **argv)
{
    const char *names[] = {"M4 2/4/8-thread realloc and synchronized display", "M4 cross-thread ownership handoff", "M4 injected corruption must fail worker"};
    int (*cases[])(void) = {m4_stress, m4_handoff, m4_failure_detection};
    return bonus_run(argc, argv, names, cases, 3);
}
