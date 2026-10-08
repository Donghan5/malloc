#include "malloc.h"
#include "test_helpers.h"
#include "bonus_helpers.h"
#include "test_ui.h"
#define CHECK(x) do { if (!(x)) { ft_putstr_fd("Edge assertion: " #x "\n", 2); return 1; } } while (0)

int thread_edge_early_return_unlock(void)
{
    unsigned char *p;
    CHECK(malloc(0) == NULL);
    free(NULL);
    CHECK(realloc(NULL, 0) == NULL);
    p = malloc(16); CHECK(p);
    ft_memset(p, 0x5a, 16);
    CHECK(realloc(p, 0) == NULL);
    p = malloc(16); CHECK(p); free(p);
    show_alloc_mem(); show_alloc_mem_ex();
    return bonus_valid_state();
}

static pthread_mutex_t first_use_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t first_use_ready = PTHREAD_COND_INITIALIZER;
static int first_use_open;

static void *first_use_worker(void *argument)
{
    size_t n = (size_t)(uintptr_t)argument;
    unsigned char *p;
    int failed;
    pthread_mutex_lock(&first_use_lock);
    while (!first_use_open) pthread_cond_wait(&first_use_ready, &first_use_lock);
    pthread_mutex_unlock(&first_use_lock);
    p = malloc(n);
    if (!p) return (void *)1;
    failed = !pattern(p, n, 0xaa);
    free(p);
    return (void *)(uintptr_t)failed;
}

int thread_edge_concurrent_first_use(void)
{
    pthread_t threads[8];
    size_t created = 0, i;
    int failed = 0;
    CHECK(setenv("MALLOC_SCRIBBLE", "1", 1) == 0);
    for (i = 0; i < 8; ++i)
    {
        if (pthread_create(&threads[i], NULL, first_use_worker, (void *)(uintptr_t)(i + 1)))
        { failed = 1; break; }
        ++created;
    }
    /* Open the gate even after a partial create failure, so created workers
     * are never stranded waiting for a fixed-size barrier. */
    pthread_mutex_lock(&first_use_lock);
    first_use_open = 1;
    pthread_cond_broadcast(&first_use_ready);
    pthread_mutex_unlock(&first_use_lock);
    for (i = 0; i < created; ++i)
    {
        void *result = NULL;
        if (pthread_join(threads[i], &result) || result) failed = 1;
    }
    CHECK(!failed && g_data.initialized == 1);
    return bonus_valid_state();
}

int thread_edge_realloc_failure_unlock(void)
{
    volatile size_t overflow = SIZE_MAX;
    unsigned char *p = malloc(32), *q;
    CHECK(p); ft_memset(p, 0x5a, 32);
    CHECK(realloc(p, overflow) == NULL && pattern(p, 32, 0x5a));
    CHECK(malloc(overflow) == NULL);
    q = malloc(16); CHECK(q); free(q); free(p);
    return bonus_valid_state();
}


int main(int argc, char **argv) {
 const char *names[] = {"thread_edge_early_return_unlock", "thread_edge_concurrent_first_use", "thread_edge_realloc_failure_unlock"};
 int (*cases[])(void) = {thread_edge_early_return_unlock, thread_edge_concurrent_first_use, thread_edge_realloc_failure_unlock};
 return bonus_run(argc, argv, names, cases, sizeof(cases)/sizeof(cases[0]));
}
