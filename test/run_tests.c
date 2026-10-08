#define _XOPEN_SOURCE 700
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "test_ui.h"

static int checks;
static int failures;

static void report(const char *name, int success)
{
    ++checks;
    failures += !success;
    test_ui_result(name, success);
    fflush(stdout);
}

/* Run children in a separate process group so timeout also stops build workers. */
static int run(char *const argv[], const char *cwd, int output, int seconds)
{
    pid_t child;
    int status;
    struct timespec start, now, pause = {0, 10000000};
    fflush(NULL);
    child = fork();
    if (child < 0) return -1;
    if (child == 0)
    {
        if (setpgid(0, 0) || (cwd && chdir(cwd))) _exit(125);
        if (output >= 0 && dup2(output, STDOUT_FILENO) < 0) _exit(125);
        execvp(argv[0], argv);
        _exit(127);
    }
    setpgid(child, child);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (;;)
    {
        pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child)
            return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
        if (result < 0 && errno != EINTR) return -1;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec - start.tv_sec >= seconds)
        {
            kill(-child, SIGKILL);
            while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
            fprintf(stderr, "Timeout: %s\n", argv[0]);
            return 124;
        }
        nanosleep(&pause, NULL);
    }
}

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *text;
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) || (length = ftell(file)) < 0
        || fseek(file, 0, SEEK_SET)) { fclose(file); return NULL; }
    text = malloc((size_t)length + 1);
    if (!text) { fclose(file); return NULL; }
    if (fread(text, 1, (size_t)length, file) != (size_t)length)
    { free(text); fclose(file); return NULL; }
    text[length] = 0;
    fclose(file);
    return text;
}

static int subtraction_audit(void)
{
    char *text = read_file("src/tools/tools.c"), *p, *q;
    int success = text != NULL;
    if (!text) return 0;
    for (p = text; *p; ++p)
    {
        if (*p != 'd' || (p > text && (isalnum((unsigned char)p[-1]) || p[-1] == '_')))
            continue;
        q = p + 1;
        while (isspace((unsigned char)*q)) ++q;
        if (*q++ != '-') continue;
        while (isspace((unsigned char)*q)) ++q;
        if (*q == 's' && !isalnum((unsigned char)q[1]) && q[1] != '_') success = 0;
    }
    free(text);
    return success;
}

static int audit_ok;
static int scan_page(const char *path, const struct stat *st, int type, struct FTW *info)
{
    const char *extension = strrchr(path, '.');
    char *text;
    (void)st; (void)info;
    if (type == FTW_F && extension && (!strcmp(extension, ".c") || !strcmp(extension, ".h")))
    {
        text = read_file(path);
        if (!text || strstr(text, "getpagesize(")) audit_ok = 0;
        free(text);
    }
    return 0;
}

static int remove_entry(const char *path, const struct stat *st, int type, struct FTW *info)
{
    (void)st; (void)type; (void)info;
    return remove(path);
}

static unsigned long long object_stamp;
static size_t object_count;
static int scan_objects(const char *path, const struct stat *st, int type, struct FTW *info)
{
    const char *extension = strrchr(path, '.');
    (void)info;
    if (type == FTW_F && extension && !strcmp(extension, ".o"))
    {
        ++object_count;
        object_stamp += (unsigned long long)st->st_mtim.tv_sec * 1000000000ULL
            + (unsigned long long)st->st_mtim.tv_nsec;
    }
    return 0;
}

static int build(const char *directory)
{
    char *args[] = {"make", "all", NULL};
    return run(args, directory, -1, 60) == 0;
}

static int same_stamp(const struct stat *a, const struct stat *b)
{
    return a->st_mtim.tv_sec == b->st_mtim.tv_sec
        && a->st_mtim.tv_nsec == b->st_mtim.tv_nsec;
}

static void build_checks(void)
{
    char directory[] = "/tmp/malloc-m1-XXXXXX";
    char library[PATH_MAX], link[PATH_MAX], objects[PATH_MAX], header[PATH_MAX];
    char target[PATH_MAX];
    struct utsname host;
    struct stat before, after, link_info;
    unsigned long long stamp;
    struct timespec times[2];
    ssize_t length;
    int ready, success;
    if (setenv("HOSTTYPE", "", 1))
    { perror("HOSTTYPE environment"); ++failures; return; }
    if (!mkdtemp(directory))
    {
        report("HOSTTYPE fallback / shared library / symlink", 0);
        report("unchanged second make", 0);
        report("header change rebuild", 0);
        return;
    }
    {
        char *args[] = {"cp", "-R", "src", "inc", "Makefile", "setup.sh", directory, NULL};
        ready = run(args, NULL, -1, 60) == 0 && uname(&host) == 0;
    }
    snprintf(objects, sizeof(objects), "%s/obj", directory);
    snprintf(header, sizeof(header), "%s/inc/define.h", directory);
    snprintf(link, sizeof(link), "%s/libft_malloc.so", directory);
    if (ready)
        snprintf(library, sizeof(library), "%s/libft_malloc_%s_%s.so", directory, host.machine, host.sysname);
    else library[0] = 0;
    ready = ready && build(directory);
    length = readlink(link, target, sizeof(target) - 1);
    if (length >= 0) target[length] = 0;
    success = ready && stat(library, &before) == 0 && S_ISREG(before.st_mode)
        && lstat(link, &link_info) == 0 && S_ISLNK(link_info.st_mode)
        && length >= 0 && !strcmp(target, strrchr(library, '/') + 1);
    report("HOSTTYPE fallback / shared library / symlink", success);
    success = ready && stat(library, &before) == 0 && build(directory)
        && stat(library, &after) == 0 && same_stamp(&before, &after);
    report("unchanged second make", success);
    object_stamp = 0; object_count = 0;
    success = ready && nftw(objects, scan_objects, 16, FTW_PHYS) == 0 && object_count > 0;
    stamp = object_stamp;
    if (success)
    {
        clock_gettime(CLOCK_REALTIME, &times[0]);
        times[0].tv_sec += 2;
        times[1] = times[0];
        success = utimensat(AT_FDCWD, header, times, 0) == 0 && build(directory);
        object_stamp = 0; object_count = 0;
        success = success && nftw(objects, scan_objects, 16, FTW_PHYS) == 0
            && object_count > 0 && object_stamp != stamp;
    }
    report("header change rebuild", success);
    if (nftw(directory, remove_entry, 16, FTW_DEPTH | FTW_PHYS))
    { perror("temporary build cleanup"); ++failures; }
}

int main(int argc, char **argv)
{
    FILE *output;
    char binary[PATH_MAX], index[32];
    unsigned int count, i;
    int m1 = argc == 3 && !strcmp(argv[2], "--m1");
    int m2 = argc == 3 && !strcmp(argv[2], "--m2");
    struct rlimit cores = {0, 0};
    if ((argc != 2 && !m1 && !m2) || !realpath(argv[1], binary)) return 1;
    if (setrlimit(RLIMIT_CORE, &cores) || unsetenv("MALLOC_DEBUG") || unsetenv("MALLOC_SCRIBBLE")) return 1;
    output = tmpfile();
    if (!output) return 1;
    {
        char *args[] = {binary, NULL};
        if (run(args, NULL, fileno(output), 10) != 0) { fclose(output); return 1; }
    }
    rewind(output);
    if (fscanf(output, "%u", &count) != 1 || count == 0 || count > 10000)
    { fclose(output); return 1; }
    fclose(output);
    test_ui_heading(m1 ? "M1" : (m2 ? "M2" : "Mandatory"));
    for (i = 0; i < count; ++i)
    {
        char *args[] = {binary, index, NULL};
        snprintf(index, sizeof(index), "%u", i);
        int result = run(args, NULL, -1, 15);
        /* Normal cases already print their named result. */
        if (result == 0 || result == 1)
        { ++checks; failures += result != 0; }
        else report(index, 0);
    }
    if (m1)
    {
        report("source audit: known cross-object pointer subtraction", subtraction_audit());
#ifdef __linux__
        audit_ok = 1;
        if (nftw("src", scan_page, 16, FTW_PHYS) || nftw("inc", scan_page, 16, FTW_PHYS)) audit_ok = 0;
        report("Linux page API source audit", audit_ok);
#endif
        build_checks();
    }
    test_ui_summary(m1 ? "M1" : (m2 ? "M2" : "Mandatory"),
        (unsigned int)(checks - failures), (unsigned int)checks);
    return failures ? 1 : 0;
}
