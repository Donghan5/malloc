#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

enum { ENTRY, PREPARED, BINDING, ALLOCATED, PAYLOAD, RELEASED, HUNDRED, END, COUNT };
/* 기록은 고정 크기이며 반복할 때 추가 배열이나 진단용 할당을 만들지 않는다. */
static struct {
    struct rusage scratch;
    long faults[COUNT];
} records __attribute__((aligned(4096)));

static int snapshot(unsigned index)
{
    if (getrusage(RUSAGE_SELF, &records.scratch)) return -1;
    records.faults[index] = records.scratch.ru_minflt;
    return 0;
}

static int bound(const char *name, void *address, const struct stat *expected)
{
    void *symbol = dlsym(RTLD_DEFAULT, name);
    Dl_info info;
    struct stat actual;
    return symbol && symbol == address && dladdr(symbol, &info)
        && info.dli_fname && stat(info.dli_fname, &actual) == 0
        && expected->st_dev == actual.st_dev && expected->st_ino == actual.st_ino;
}

static int verify(void)
{
    const char *library = getenv("EXPECTED_MALLOC_LIBRARY");
    struct stat expected;
    if (!library || library[0] != '/' || stat(library, &expected)
        || !bound("malloc", (void *)malloc, &expected)
        || !bound("free", (void *)free, &expected)
        || getenv("MALLOC_DEBUG") || getenv("MALLOC_SCRIBBLE")) {
        fputs("FAIL: 라이브러리 주입 또는 측정 환경 확인 실패\n", stderr);
        return -1;
    }
    return 0;
}

static void output(int repeat)
{
    const char *names[COUNT] = {"초기", "준비", "주입확인", "첫malloc",
        "payload", "첫free", "100회", "1024회"};
    unsigned checkpoints[] = {BINDING, RELEASED, HUNDRED, END};
    unsigned iterations[] = {0, 1, 100, 1024};
    unsigned count = repeat ? 4 : COUNT;
    long baseline = records.faults[repeat ? BINDING : ENTRY], previous = baseline;
    puts("구간/반복 ru_minflt 기준대비 직전증가");
    for (unsigned i = 0; i < count; ++i) {
        unsigned index = repeat ? checkpoints[i] : i;
        long faults = records.faults[index];
        if (repeat) printf("%u", iterations[i]);
        else printf("%s", names[i]);
        printf(" %ld %ld %ld\n", faults, faults - baseline, faults - previous);
        previous = faults;
    }
}

int main(int argc, char **argv)
{
    /* 첫 getrusage 자체를 미리 호출해 비용을 숨기지 않는다. */
    if (snapshot(ENTRY)) return 4;
    long entry = records.faults[ENTRY];
    volatile unsigned char *storage = (volatile unsigned char *)&records;
    for (size_t i = 0; i < sizeof(records); ++i) storage[i] = 0;
    records.faults[ENTRY] = entry;
    alarm(10);
    if (snapshot(PREPARED)) return 4;
    if (argc != 2 || (strcmp(argv[1], "binding") && strcmp(argv[1], "repeat")
        && strcmp(argv[1], "baseline") && strcmp(argv[1], "free"))) {
        fputs("사용법: fault_probe binding|repeat|baseline|free\n", stderr);
        return 2;
    }
    if (verify()) return 2;
    if (snapshot(BINDING)) return 4;
    if (!strcmp(argv[1], "binding")) {
        puts("PASS: malloc/free 주소와 라이브러리 device/inode 확인");
        return ferror(stdout) ? 5 : 0;
    }
    int repeat = !strcmp(argv[1], "repeat");
    int allocate = strcmp(argv[1], "baseline") != 0;
    for (unsigned i = 1; i <= 1024; ++i) {
        void *p = NULL;
        if (allocate) {
            p = malloc(1024);
            if (!p) return 3;
        }
        if (!repeat && i == 1 && snapshot(ALLOCATED)) return 4;
        if (allocate) {
            volatile unsigned char *bytes = p;
            for (size_t j = 0; j < 1024; ++j) bytes[j] = (unsigned char)(i - 1 + j);
        }
        if (!repeat && i == 1 && snapshot(PAYLOAD)) return 4;
        if (allocate) free(p);
        if ((i == 1 && snapshot(RELEASED)) || (i == 100 && snapshot(HUNDRED))
            || (i == 1024 && snapshot(END))) return 4;
    }
    /* 출력과 출력용 배열 준비도 마지막 snapshot 이후에만 한다. */
    output(repeat);
    return ferror(stdout) ? 5 : 0;
}
