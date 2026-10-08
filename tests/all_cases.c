#include "malloc.h"
#include "all_cases.h"

int run_isolated(char *const argv[], int enabled)
{
    char path[] = "/tmp/malloc-review-XXXXXX";
    char output[4096];
    ssize_t n, i;
    int fd = mkstemp(path), status;
    pid_t child;
    if (fd < 0) return 1;
    unlink(path);
    child = fork();
    if (child < 0) { close(fd); return 1; }
    if (!child)
    {
        struct rlimit limit = {0, 0};
        setrlimit(RLIMIT_CORE, &limit);
        if (dup2(fd, 1) < 0 || dup2(fd, 2) < 0) _exit(125);
        close(fd);
        if (enabled) { setenv("MALLOC_DEBUG", "1", 1); setenv("MALLOC_SCRIBBLE", "1", 1); }
        else { unsetenv("MALLOC_DEBUG"); unsetenv("MALLOC_SCRIBBLE"); }
        alarm(20);
        execv(argv[0], argv);
        _exit(127);
    }
    if (waitpid(child, &status, 0) != child) { close(fd); return 1; }
    if (lseek(fd, 0, SEEK_SET) < 0) { close(fd); return 1; }
    n = read(fd, output, sizeof(output) - 1);
    close(fd);
    if (n < 0) return 1;
    output[n] = 0;
    for (i = 0; i < n && output[i] != '\n'; ++i) {}
    if (status == 0) output[i] = 0;
    ft_putstr_fd(output, 1); ft_putstr_fd("\n", 1);
    return status != 0;
}

int main(int argc, char **argv)
{
    const char *binaries[] = {"./tests/m1_cases", "./tests/m2_cases", "./tests/m3_cases", "./tests/m4_cases", "./tests/m5_cases"};
    const size_t counts[] = {25, 9, 5, 3, 2};
    size_t group, i, pass = 0, total = 0;
    int result, first = 0, last = 5;
    char index[16];
    if (argc == 2 && argv[1][0] >= '1' && argv[1][0] <= '5' && !argv[1][1])
    { first = argv[1][0] - '1'; last = first + 1; }
    else if (argc != 1) return 1;
    ft_putstr_fd("\n========== MALLOC M1–M5 ==========\n", 1);
    for (group = (size_t)first; group < (size_t)last; ++group)
    {
        ft_putstr_fd("\nMilestone M", 1); ft_print_unsigned_fd(group + 1, 1); ft_putstr_fd("\n", 1);
        for (i = 0; i < counts[group]; ++i)
        {
            char *args[] = {(char *)binaries[group], index, NULL};
            index[0] = (char)('0' + i / 10); index[1] = (char)('0' + i % 10); index[2] = 0;
            result = run_isolated(args, 0);
            ++total; pass += !result;
            ft_putstr_fd(result ? "\033[31m  FAIL\033[0m\n" : "\033[32m  PASS\033[0m\n", 1);
        }
        if (group == 0)
        {
            char *audit[] = {"./tests/test_runner", "./tests/m1_cases", "--m1", NULL};
            ft_putstr_fd("M1 build contracts and source audits\n", 1);
            result = run_isolated(audit, 0); ++total; pass += !result;
            ft_putstr_fd(result ? "\033[31m  FAIL\033[0m\n" : "\033[32m  PASS\033[0m\n", 1);
            /* Repeat all mandatory contracts with bonus flags enabled. */
            for (i = 0; i < 16; ++i)
            {
                char *args[] = {(char *)binaries[group], index, NULL};
                index[0] = (char)('0' + i / 10); index[1] = (char)('0' + i % 10); index[2] = 0;
                ft_putstr_fd("[debug/scribble enabled] ", 1);
                result = run_isolated(args, 1); ++total; pass += !result;
                ft_putstr_fd(result ? "\033[31m  FAIL\033[0m\n" : "\033[32m  PASS\033[0m\n", 1);
            }
        }
    }
    ft_putstr_fd("\nResult: ", 1); ft_print_unsigned_fd(pass, 1); ft_putstr_fd("/", 1);
    ft_print_unsigned_fd(total, 1); ft_putstr_fd(" passed\n", 1);
    return pass != total;
}
