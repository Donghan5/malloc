#include "test_helpers.h"
#include "bonus_helpers.h"
#include "m3_cases.h"
#define CHECK(x) do { if (!(x)) { ft_putstr_fd("Assertion: " #x "\n", 2); return 1; } } while (0)

/* Contract follows init_debug_flags: debug is presence-based and cached;
 * scribble is refreshed per operation and enabled unless first byte is '0'. */
int m3_environment(void)
{
    const char *values[] = {NULL, "0", "1", ""};
    size_t i;
    int saved, fd;
    char output[4096];
    fd = capture_begin(&saved); CHECK(fd >= 0);
    for (i = 0; i < 4; ++i)
    {
        if (values[i]) CHECK(setenv("MALLOC_DEBUG", values[i], 1) == 0);
        else CHECK(unsetenv("MALLOC_DEBUG") == 0);
        g_data.initialized = 0;
        free(NULL);
        CHECK(g_data.debug == (values[i] != NULL));
        CHECK(unsetenv("MALLOC_DEBUG") == 0);
        free(NULL);
        CHECK(g_data.debug == (values[i] != NULL));
        if (values[i]) CHECK(setenv("MALLOC_SCRIBBLE", values[i], 1) == 0);
        else CHECK(unsetenv("MALLOC_SCRIBBLE") == 0);
        free(NULL);
        CHECK(g_data.scribble == (values[i] && values[i][0] != '0'));
    }
    CHECK(capture_end(fd, saved, output, sizeof(output)) == 0);
    return 0;
}

int m3_debug_logs(void)
{
    volatile size_t overflow = SIZE_MAX;
    char output[4096];
    unsigned char *p, *q;
    int fd, saved;
    CHECK(setenv("MALLOC_DEBUG", "1", 1) == 0);
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

int m3_scribble(void)
{
    unsigned char *p, *q;
    CHECK(setenv("MALLOC_SCRIBBLE", "1", 1) == 0);
    p = malloc(37); CHECK(p && pattern(p, 37, 0xaa));
    ft_memset(p, 0x12, 37);
    q = realloc(p, SMALL_BLOCK_SIZE + 64);
    CHECK(q && pattern(q, 37, 0x12));
    free(q);
    /* realloc(NULL,n) is an allocation and must honor allocation scribble. */
    p = realloc(NULL, 37); CHECK(p && pattern(p, 37, 0xaa));
    free(p);
    return 0;
}

int m3_hex_rows(void)
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

int m3_extended_output(void)
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

int main(int argc, char **argv)
{
    const char *names[] = {"M3 environment values and caching", "M3 success/failure debug logs", "M3 allocation and realloc scribble", "M3 hex row boundaries", "M3 empty and live extended output"};
    int (*cases[])(void) = {m3_environment, m3_debug_logs, m3_scribble, m3_hex_rows, m3_extended_output};
    return bonus_run(argc, argv, names, cases, 5);
}
