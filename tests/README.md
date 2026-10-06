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

The C test source includes only `inc/malloc.h`, which provides the project's
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
16 mandatory cases and adds six runtime cases in `m1_cases.h`:

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

Latest result: **22/27 checks passed, five failed**:

- Same-class fragmented capacity: the larger request returns NULL.
- Deterministic heap output ordering: descending addresses are printed.
- Known cross-object pointer subtraction remains in `ft_memmove`.
- Linux sources still use `getpagesize()`.
- Changing `inc/define.h` does not rebuild shared-library objects.

The allocator is deliberately left unchanged by this test addition. M1 remains
incomplete. Manual review is still needed for the documented justification of
bonus-only functions such as `getenv`, complete C portability, and supported OS
contracts. The forced-order test covers LARGE heap headers; exhaustive mixed-class
and block-address output validation is not claimed.

## Upcoming Feature — Bonus Part

The following items are reserved for future work and are not tested by this suite:

- pthread-based thread safety and concurrency stress tests.
- Validation of the `MALLOC_DEBUG` and `MALLOC_SCRIBBLE` environment variables.
- Validation of the `show_alloc_mem_ex` hex dump.
- Defragmentation: merging adjacent free blocks and reusing space left by realloc shrinking.

Existing bonus code does not count as verified bonus functionality in this suite.
