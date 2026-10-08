/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   all_cases.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 21:35:37 by donghank          #+#    #+#             */
/*   Updated: 2026/10/08 21:35:37 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include "all_cases.h"
#include "test_helpers.h"
#include "test_ui.h"

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

int suite_count(const char *binary, size_t *count)
{
    char path[] = "/tmp/malloc-count-XXXXXX", output[32];
    int fd = mkstemp(path), status;
    pid_t child;
    ssize_t n;
    uintptr_t value;
    const char *cursor = output;
    if (fd < 0) return 1;
    unlink(path);
    child = fork();
    if (child < 0) { close(fd); return 1; }
    if (!child)
    {
        char *args[] = {(char *)binary, NULL};
        if (dup2(fd, 1) < 0) _exit(125);
        close(fd);
        alarm(10);
        execv(binary, args);
        _exit(127);
    }
    if (waitpid(child, &status, 0) != child || status || lseek(fd, 0, SEEK_SET) < 0)
    { close(fd); return 1; }
    n = read(fd, output, sizeof(output) - 1); close(fd);
    if (n <= 0 || n == (ssize_t)sizeof(output) - 1) return 1;
    output[n] = 0;
    if (!parse_number(&cursor, 10, &value) || *cursor != '\n' || cursor[1]
        || !value || value > 10000) return 1;
    *count = (size_t)value;
    return 0;
}

static void index_text(size_t value, char *output)
{
    char reverse[32];
    size_t length = 0, i;
    do { reverse[length++] = (char)('0' + value % 10); value /= 10; } while (value);
    for (i = 0; i < length; ++i) output[i] = reverse[length - i - 1];
    output[length] = 0;
}

int main(int argc, char **argv)
{
    const char *binaries[] = {"./test/bin/m1_cases", "./test/bin/m2_cases", "./test/bin/m3_cases", "./test/bin/m4_cases", "./test/bin/m5_cases", "./test/bin/edge_cases"};
    size_t count;
    size_t group, i, pass = 0, total = 0;
    int result, first = 0, last = 6;
    char index[32];
    if (argc == 2 && argv[1][0] >= '1' && argv[1][0] <= '5' && !argv[1][1])
    { first = argv[1][0] - '1'; last = first + 1; }
    else if (argc == 2 && starts_with(argv[1], "edge") && argv[1][4] == 0)
    { first = 5; last = 6; }
    else if (argc != 1) return 1;
    ft_putstr_fd("\n========== MALLOC M1–M5 ==========\n", 1);
    for (group = (size_t)first; group < (size_t)last; ++group)
    {
        if (group == 5) test_ui_heading("Edge cases M1–M5");
        else { ft_putstr_fd("\nMilestone M", 1); ft_print_unsigned_fd(group + 1, 1); ft_putstr_fd("\n", 1); }
        if (suite_count(binaries[group], &count))
        { ++total; test_ui_result("cannot read suite count", 0); continue; }
        for (i = 0; i < count; ++i)
        {
            char *args[] = {(char *)binaries[group], index, NULL};
            index_text(i, index);
            result = run_isolated(args, 0);
            ++total; pass += !result;
            ft_putstr_fd(result ? "\033[31m  FAIL\033[0m\n" : "\033[32m  PASS\033[0m\n", 1);
        }
        if (group == 0)
        {
            char *audit[] = {"./test/bin/test_runner", "./test/bin/m1_cases", "--m1", NULL};
            ft_putstr_fd("M1 build contracts and source audits\n", 1);
            result = run_isolated(audit, 0); ++total; pass += !result;
            ft_putstr_fd(result ? "\033[31m  FAIL\033[0m\n" : "\033[32m  PASS\033[0m\n", 1);
            /* Repeat all mandatory contracts with bonus flags enabled. */
            for (i = 0; i < 16; ++i)
            {
                char *args[] = {(char *)binaries[group], index, NULL};
                index_text(i, index);
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
