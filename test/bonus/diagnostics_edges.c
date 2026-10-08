#include "malloc.h"
#include "test_helpers.h"
#include "bonus_helpers.h"
#include "test_ui.h"
#define CHECK(x) do { if (!(x)) { ft_putstr_fd("Edge assertion: " #x "\n", 2); return 1; } } while (0)

int diagnostic_edge_cached_scribble(void)
{
 unsigned char *p;
 CHECK(unsetenv("MALLOC_SCRIBBLE") == 0);
 p = malloc(16); CHECK(p && g_data.scribble == 0); free(p);
 CHECK(setenv("MALLOC_SCRIBBLE", "1", 1) == 0);
 p = malloc(16); CHECK(p && g_data.scribble == 0); free(p);
 fixture_reset_flags();
 p = malloc(16); CHECK(p && pattern(p, 16, 0xaa)); free(p);
 CHECK(setenv("MALLOC_SCRIBBLE", "", 1) == 0);
 p = malloc(16); CHECK(p && pattern(p, 16, 0xaa)); free(p);
 return 0;
}

int diagnostic_edge_realloc_null_scribble(void)
{
    unsigned char *p;
    CHECK(setenv("MALLOC_SCRIBBLE", "1", 1) == 0);
    CHECK(realloc(NULL, 0) == NULL);
    p = realloc(NULL, 1);
    CHECK(p && pattern(p, 1, 0xaa));
    free(p);
    return 0;
}

int diagnostic_edge_freed_dump_exclusion(void)
{
    unsigned char *p = malloc(32), *guard = malloc(32);
    char output[8192];
    int saved, fd;
    CHECK(p && guard);
    ft_memset(p, 0xab, 32); ft_memset(guard, 0x12, 32);
    free(p);
    fd = capture_begin(&saved); CHECK(fd >= 0);
    show_alloc_mem_ex();
    CHECK(capture_end(fd, saved, output, sizeof(output)) == 0);
    CHECK(!find_text(output, "ab ab ab ab"));
    CHECK(find_text(output, "12 12 12 12"));
    CHECK(find_text(output, "Total : 32 bytes\n"));
    CHECK(pattern(guard, 32, 0x12)); free(guard);
    return 0;
}


int main(int argc, char **argv) {
 const char *names[] = {"diagnostic_edge_cached_scribble", "diagnostic_edge_realloc_null_scribble", "diagnostic_edge_freed_dump_exclusion"};
 int (*cases[])(void) = {diagnostic_edge_cached_scribble, diagnostic_edge_realloc_null_scribble, diagnostic_edge_freed_dump_exclusion};
 return bonus_run(argc, argv, names, cases, sizeof(cases)/sizeof(cases[0]));
}
