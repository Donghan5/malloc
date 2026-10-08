# Mandatory Feature Edge Case Tests

Run from the project root:

```sh
make test
```

The suite requires a C compiler, POSIX process APIs, `make`, and pthread support on Linux/POSIX. It builds the
test executable directly from source, independently of `main.c` and the shared
library build. The executable is excluded from Git tracking.

## Execution and Results

- `tests/run_tests.c` starts a separate executable process for each test. The
  remaining tests continue even if one terminates with SIGSEGV.
- Each test has a 10-second timeout. Timeouts count as failures. Core dumps are disabled.
- Each test returns 0 on success and 1 on assertion failure. The C runner
  also counts signal termination as a failure and returns 1 if any test fails.
  `make` propagates failures through its own nonzero exit status.
- Allocator functions are renamed to `edge_malloc`, `edge_free`, and `edge_realloc`
  at compile time so that test output and libc allocations do not affect the
  allocator's global heap state. LD_PRELOAD integration and integration with
  system programs are outside the scope of this suite.
- The failure test that sets `RLIMIT_AS` to 0 affects only that test process.
- Tests for `malloc(0)` and `realloc(ptr, 0)` check this project's NULL-return policy.

## Headers and Helper Functions

The C tests use `test_helpers.h` and `inc/malloc.h`, which provide the project's
headers and its existing system library declarations. Output uses
`ft_putstr_fd` and `ft_print_unsigned_fd`; memory initialization uses `ft_memset`.
Small local helpers compare strings, parse decimal and hexadecimal values, and
check byte patterns without adding standard library dependencies such as
`stdio.h` or `string.h`. Output capture and resource limits use the POSIX APIs
already declared through `malloc.h`. Process isolation and result aggregation
are handled by the C runner.

Running `./tests/edge_cases` prints the test count. Passing a zero-based index,
for example `./tests/edge_cases 0`, runs a single test. Use `make test` to run
the complete suite with crash isolation and core file suppression.

## Coverage

The 16 tests cover:

- Zero-byte requests, freeing NULL, and `realloc(NULL, size)`.
- 16-byte alignment, size class and page boundaries, and data preservation across
  allocations that are live at the same time.
- Correct classification of TINY requests when a SMALL heap already exists.
- Overflow during size alignment, metadata addition, and page rounding.
- Same-size reallocation, shrinking, growth across size classes, and data preservation.
- Preservation of the original pointer and data when realloc overflows or mmap fails.
- Lifetime of a separate allocation after shrinking a LARGE allocation, with both
  orders of freeing the allocations.
- The minimum remaining space required when splitting a block.
- Successful allocation and data preservation across multiple heaps and fragmented
  memory using 400 allocations.
- Removal of middle, head, and tail nodes from the LARGE heap list.
- `show_alloc_mem` totals and ascending heap address output.

The address ordering test depends on the actual mmap layout. Passing one run does
not prove correct ordering for every possible address layout. Undefined C usage,
such as freeing invalid pointers, double frees, and accesses outside user buffers,
is excluded from the mandatory contract tests.

## Current Results

The implementation passed **16 of 16 tests** on 2026-10-06 with `make test`.
The LARGE lifetime and size class isolation regressions now pass.
This result covers this suite only; see the stricter M1 checks below.

## M1 Contract Tests

```sh
make test-m1
```

The runner is written in C and compiled as `tests/test_runner`; Python and shell
test scripts are not required. Isolated build checks invoke `make` and `cp`. The target reuses all
16 mandatory cases and adds nine runtime cases implemented in `m1_cases.c` (`m1_cases.h` contains only function declarations):

- 100 simultaneous maximum-size allocations in a single TINY or SMALL zone,
  with data preservation and `max_align_t` alignment checks.
- Exhaustion of a single TINY heap followed by isolated 16-byte holes: aggregate
  free space must not prevent a larger TINY request from creating a new heap.
- `ft_memmove` forward/backward overlap, independent objects, self-copy and zero length.
- Ascending diagnostic heap addresses with an explicitly descending internal list,
  independent of mmap address order, plus the live allocation total.
- Printed page size compared with `sysconf(_SC_PAGESIZE)`.

`run_tests.c` runs every runtime case in a separate process with a 15-second outer
timeout (the C harness also has a 10-second alarm). Debug/scribble variables are
removed from the child environment. Failures and signal termination propagate to
`make` as a nonzero exit status. Builds run in a temporary copy, leaving repository
build artifacts and headers untouched. Three build checks cover empty HOSTTYPE
fallback, library naming/symlink, an unchanged second build, and rebuilding after
an `inc/define.h` change (naming/symlink/fallback form one check).

Two narrowly scoped source audits flag the known `d - s` expression and Linux
`getpagesize()` calls. These checks are explicitly source audits, not runtime
proofs of undefined behavior or complete allowed-function validation. Passing
functional memmove tests alone does not prove defined C behavior.

Latest result (2026-10-07): **30/30 checks passed**, exit status 0, on
Fedora Linux 44 x86_64 with a 4096-byte page size. All five previously failing
checks now pass. This is a local Fedora result, not a run on the school's machine.

The three added cases verify mixed TINY/SMALL/LARGE heap and block address order,
exact payload endpoints and sizes, totals, freed holes, removed heaps, initial
empty output and output after all frees; and page-aligned mappings, TINY/SMALL
zone sizes, LARGE metadata-inclusive page rounding, overflow and the extended
page-size output. Expected page sizes come from `sysconf`, not a 4096 constant.
Output capture uses an unlinked temporary file to avoid pipe capacity deadlocks.

Manual review of the justification for bonus-only functions such as `getenv`
remains. The school's target is Fedora/Linux; other OS execution is outside this
verification. This suite does not prove every possible C portability property
or validate the extended hex dump or concurrency contracts.

## M2 Defragmentation Tests

```sh
make test-m2
```

`m2_cases.c` implements nine cases; `m2_cases.h` contains function prototypes
only. Each case runs in its own process with the existing runner, a 15-second
outer timeout and a 10-second alarm. Any assertion, crash or timeout makes the
target fail. To run one case directly, use `./tests/m2_cases 0` (indices 0–8).

Coverage includes the milestone's exact 128/64/64 regression, TINY and SMALL
shrinking, repeated shrinking, forward/backward/both-side merging, recovery of
removed header space, reuse by larger requests, first/last block boundaries,
and three-zone allocation/free cycles in both orders for both groups.
State inspection checks physical block coverage, prev/next links, absence of
adjacent free blocks, block_count, free_size and global group counts. Live
payload patterns are checked throughout; freed payloads are never read.
The empty-zone test permits keeping the final empty zone or releasing it,
while requiring extra empty zones to be reclaimed.

Local result (2026-10-07): **5/9 passed, 4 failed**. The required regression,
TINY/SMALL shrink and repeated-shrink cases detect adjacent free blocks after
realloc shrinking. These are allocator failures; M2 is not complete.

## M3–M5 and evaluator suite

```sh
make test-m3   # environment, logging, scribble, hex dump
make test-m4   # threads, pointer handoff, injected corruption
make test-all  # M1–M5 plus M1 build/source checks and enabled mandatory cases
make test-m5   # same complete submission gate
```

New suites are split into `.c` implementations and prototype-only `.h` files.
They include only project headers from `inc` through the helper headers. The
existing M1 build runner retains its existing system headers. New output uses
`ft_putstr_fd` and `ft_print_unsigned_fd`, with green PASS and red FAIL markers.
Each runtime test is a separate process, with a 10-second alarm and a 20-second
exec-level alarm. Nonzero exit, crashes and timeouts fail the aggregate, while
remaining tests continue. Captured child output is shown briefly on success
and up to 4095 bytes on failure. Run commands from the repository root.

M3 checks the current environment interpretation: MALLOC_DEBUG is enabled by
presence (including `0` and empty), cached on first initialization;
MALLOC_SCRIBBLE is refreshed on each operation, enabled by presence unless its
first character is `0` (empty enables it). Ordinary malloc scribbles requested
bytes with `0xaa`. The test requires realloc(NULL,n) to honor that allocation
contract. Free payloads are never read to inspect `0xdd`; pre-unmap scribble
instrumentation and exact expanded-tail behavior remain unverified.
Hex checks cover 16-byte rows, a partial row, zero length, empty extended output,
and a known live pattern and total. Exhaustive extended address-order and
all log argument checks remain outside these added cases.

M4 exercises 2, 4 and 8 workers with 120 iterations per worker across size
classes, growth and shrinking. A separate mutex prevents payload writes from
racing with dump reads. Realloc/free operations on independently owned pointers
still execute concurrently. Only successfully created threads are joined.
Heap links, physical block coverage, counts, free-size accounting and adjacent
free blocks are checked after joining. The handoff case publishes a completed
payload through pthread_create before another thread validates and frees it.
An injected corruption case requires four distinct worker failures to reach
the harness; it passes only when those failures are detected.

M5 repeats all 16 mandatory cases with debug/scribble enabled and includes a
bonus-enabled growth/overflow regression and retained-empty-zone reuse check.
The latter checks reuse through mapped metadata, not mmap/munmap syscall counts;
actual syscall instrumentation, LD_PRELOAD integration and other operating
systems remain unverified. The legacy M1 runtime/build/source suite is also
included as an aggregate check, so its 30 checks are counted as one additional
item in the evaluator total. Running `tests/m5_cases` alone checks only the two
M5-specific runtime cases, not the complete gate.

Local validation on 2026-10-08 (Linux): **55/61 aggregate items passed**.
Failures: four M2 shrink/coalescing regressions, M3 realloc(NULL,n) scribble,
and M4 adjacent free blocks after concurrent realloc shrinking. These are
reported as failures; the aggregate returns 1 and make returns a nonzero status.
This does not certify completion of M2–M4 or the submission requirements.
