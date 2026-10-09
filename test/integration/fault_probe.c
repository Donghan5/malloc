#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

enum { ENTRY, PREPARED, BINDING, EMPTY_BEGIN, EMPTY_END, ALLOCATED,
    PAYLOAD, RELEASED, HUNDRED, END, EMPTY_FINAL_BEGIN, EMPTY_FINAL_END, COUNT };
static struct {
    struct rusage scratch;
    long faults[COUNT];
} records __attribute__((aligned(4096)));

static int snapshot(unsigned index)
{
    if (getrusage(RUSAGE_SELF, &records.scratch)) {
        perror("FAIL: getrusage");
        return -1;
    }
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
    const char *names[COUNT] = {"초기", "준비", "주입확인", "빈구간시작",
        "첫malloc전", "첫malloc후/쓰기전", "쓰기후/free전", "첫free후",
        "누적100회", "누적1024회", "마지막빈구간시작", "마지막빈구간끝"};
    long baseline = records.faults[ENTRY], previous = baseline;
    puts("snapshot ru_minflt 초기대비 직전증가");
    for (unsigned index = 0; index < COUNT; ++index) {
        if (!repeat && (index == HUNDRED || index == END)) continue;
        long faults = records.faults[index];
        printf("%s %ld %ld %ld\n", names[index], faults,
            faults - baseline, faults - previous);
        previous = faults;
    }
    printf("구간 빈전=%ld malloc=%ld 쓰기=%ld free=%ld",
        records.faults[EMPTY_END] - records.faults[EMPTY_BEGIN],
        records.faults[ALLOCATED] - records.faults[EMPTY_END],
        records.faults[PAYLOAD] - records.faults[ALLOCATED],
        records.faults[RELEASED] - records.faults[PAYLOAD]);
    if (repeat) printf(" 2~100=%ld 101~1024=%ld",
        records.faults[HUNDRED] - records.faults[RELEASED],
        records.faults[END] - records.faults[HUNDRED]);
    else printf(" 2~100=미측정 101~1024=미측정");
    printf(" 빈후=%ld\n", records.faults[EMPTY_FINAL_END] - records.faults[EMPTY_FINAL_BEGIN]);
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
    if (argc != 2 || (strcmp(argv[1], "binding") && strcmp(argv[1], "cold") && strcmp(argv[1], "repeat")
        && strcmp(argv[1], "baseline") && strcmp(argv[1], "free")
        && strcmp(argv[1], "fullwrite"))) {
        fputs("사용법: fault_probe binding|cold|repeat|baseline|free|fullwrite\n", stderr);
        return 2;
    }
    if (verify()) return 2;
    if (snapshot(BINDING)) return 4;
    if (!strcmp(argv[1], "binding")) {
        puts("PASS: malloc/free 주소와 라이브러리 device/inode 확인");
        return ferror(stdout) ? 5 : 0;
    }
    int repeat = strcmp(argv[1], "cold") != 0;
    int allocate = strcmp(argv[1], "baseline") != 0;
    /* 공식 correction(test2)은 addr[0]만 쓴다. 전체 payload 쓰기는 fullwrite 모드로 분리한다. */
    int full = !strcmp(argv[1], "fullwrite");
    if (snapshot(EMPTY_BEGIN) || snapshot(EMPTY_END)) return 4;
    for (unsigned i = 1; i <= (repeat ? 1024U : 1U); ++i) {
        void *p = NULL;
        if (allocate) {
            p = malloc(1024);
            if (!p) { fputs("FAIL: malloc(1024) 실패\n", stderr); return 3; }
        }
        if (i == 1 && snapshot(ALLOCATED)) return 4;
        if (allocate) {
            volatile unsigned char *bytes = p;
            if (full) { for (size_t j = 0; j < 1024; ++j) bytes[j] = (unsigned char)(i - 1 + j); }
            else bytes[0] = 42;
        }
        if (i == 1 && snapshot(PAYLOAD)) return 4;
        if (allocate) free(p);
        if ((i == 1 && snapshot(RELEASED)) || (i == 100 && snapshot(HUNDRED))
            || (i == 1024 && snapshot(END))) return 4;
    }
    if (snapshot(EMPTY_FINAL_BEGIN) || snapshot(EMPTY_FINAL_END)) return 4;
    /* 출력과 출력용 배열 준비도 마지막 snapshot 이후에만 한다. */
    output(repeat);
    return ferror(stdout) ? 5 : 0;
}
