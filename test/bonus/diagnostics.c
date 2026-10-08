#include "malloc.h"
#include "test_helpers.h"
#include "bonus_helpers.h"
#include "test_ui.h"
#define CHECK(x) do { if (!(x)) { ft_putstr_fd("Assertion: " #x "\n", 2); return 1; } } while (0)

/* Both flags are enabled only by "1" and cached after initialization. */
int diagnostics_environment(void)
{
    const char *values[] = {NULL, "0", "", "10", "1"};
    const int enabled[] = {0, 0, 0, 0, 1};
    size_t i, j;
    int saved, fd;
    char output[4096];
    fd = capture_begin(&saved); CHECK(fd >= 0);
    for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        for (j = 0; j < sizeof(values) / sizeof(values[0]); ++j)
        {
            if (values[i]) CHECK(setenv("MALLOC_DEBUG", values[i], 1) == 0);
            else CHECK(unsetenv("MALLOC_DEBUG") == 0);
            if (values[j]) CHECK(setenv("MALLOC_SCRIBBLE", values[j], 1) == 0);
            else CHECK(unsetenv("MALLOC_SCRIBBLE") == 0);
            fixture_reset_flags();
            free(NULL);
            CHECK(g_data.initialized == 1);
            CHECK(g_data.debug == enabled[i]);
            CHECK(g_data.scribble == enabled[j]);
            CHECK(setenv("MALLOC_DEBUG", enabled[i] ? "0" : "1", 1) == 0);
            CHECK(setenv("MALLOC_SCRIBBLE", enabled[j] ? "0" : "1", 1) == 0);
            free(NULL);
            CHECK(g_data.debug == enabled[i]);
            CHECK(g_data.scribble == enabled[j]);
            CHECK(unsetenv("MALLOC_DEBUG") == 0);
            CHECK(unsetenv("MALLOC_SCRIBBLE") == 0);
            free(NULL);
            CHECK(g_data.debug == enabled[i]);
            CHECK(g_data.scribble == enabled[j]);
        }
    }
    CHECK(capture_end(fd, saved, output, sizeof(output)) == 0);
    return 0;
}

int diagnostics_debug_logs(void)
{
    volatile size_t overflow = SIZE_MAX;
    char output[4096];
    unsigned char *p, *q;
    int fd, saved;
    CHECK(setenv("MALLOC_DEBUG", "1", 1) == 0);
    fixture_reset_flags();
    fd = capture_begin(&saved); CHECK(fd >= 0);
    p = malloc(32);
    q = realloc(p, 64);
    if (q) free(q); else free(p);
    free(NULL);
    p = malloc(overflow);
    q = realloc(NULL, overflow);
    CHECK(capture_end(fd, saved, output, sizeof(output)) == 0);
    CHECK(!p && !q);
    CHECK(find_text(output, " : 32 bytes\n"));
    CHECK(find_text(output, "[MALLOC] realloc("));
    CHECK(find_text(output, "[MALLOC] free("));
    CHECK(find_text(output, "0x0000000000000000"));
    return 0;
}

int diagnostics_scribble(void)
{
    unsigned char *p, *q;
    CHECK(setenv("MALLOC_SCRIBBLE", "1", 1) == 0);
    fixture_reset_flags();
    p = malloc(37); CHECK(p && pattern(p, 37, 0xaa));
    ft_memset(p, 0x12, 37);
    q = realloc(p, SMALL_BLOCK_SIZE + 64);
    CHECK(q && pattern(q, 37, 0x12));
    free(q);
    /* realloc(NULL,n) is an allocation and must honor allocation scribble. */
    p = realloc(NULL, 37); CHECK(p); CHECK(pattern(p, 37, 0xaa));
    free(p);
    return 0;
}

int diagnostics_hex_rows(void)
{
    unsigned char bytes[17];
    char output[256];
    int fd, saved;
    ft_memset(bytes, 0xab, sizeof(bytes)); bytes[16] = 0x12;
    fd = capture_begin(&saved); CHECK(fd >= 0);
    ft_print_hex_dump(bytes, 17);
    ft_print_hex_dump(bytes, 0);
    CHECK(capture_end(fd, saved, output, sizeof(output)) == 0);
    CHECK(starts_with(output, "ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab \n12 \n"));
    CHECK(ft_strlen(output) == 53);
    return 0;
}

int diagnostics_extended_output(void)
{
    char output[8192];
    unsigned char *p;
    int fd, saved;
    fd = capture_begin(&saved); CHECK(fd >= 0);
    show_alloc_mem_ex();
    CHECK(capture_end(fd, saved, output, sizeof(output)) == 0);
    CHECK(find_text(output, "Total : 0 bytes\n"));
    p = malloc(32); CHECK(p); ft_memset(p, 0x12, 32);
    fd = capture_begin(&saved); CHECK(fd >= 0);
    show_alloc_mem_ex();
    CHECK(capture_end(fd, saved, output, sizeof(output)) == 0);
    CHECK(find_text(output, "Total : 32 bytes\n"));
    CHECK(find_text(output, "12 12 12 12 12 12 12 12 12 12 12 12 12 12 12 12 \n"));
    CHECK(pattern(p, 32, 0x12)); free(p);
    return 0;
}


int main(int argc, char **argv) {
 const char *names[] = {"diagnostics_environment", "diagnostics_debug_logs", "diagnostics_scribble", "diagnostics_hex_rows", "diagnostics_extended_output"};
 int (*cases[])(void) = {diagnostics_environment, diagnostics_debug_logs, diagnostics_scribble, diagnostics_hex_rows, diagnostics_extended_output};
 return bonus_run(argc, argv, names, cases, sizeof(cases)/sizeof(cases[0]));
}
