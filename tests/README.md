# Mandatory Feature Edge Case Tests

Run from the project root:

```sh
make test
```

The suite requires a C compiler, a POSIX shell, and pthread support on Linux/POSIX. It builds the
test executable directly from source, independently of `main.c` and the shared
library build. The executable is excluded from Git tracking.

## Execution and Results

- `tests/run_tests.sh` starts a separate executable process for each test. The
  remaining tests continue even if one terminates with SIGSEGV.
- Each test has a 10-second timeout. Timeouts count as failures. Core dumps are disabled.
- Each test returns 0 on success and 1 on assertion failure. The shell runner
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
are handled by the shell runner.

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

The current implementation passed **13 of 16 tests**.

- `size class isolation`: fails because a TINY request is placed in an existing SMALL heap.
- `LARGE shrink: free original first`: SIGSEGV when accessing a separate allocation
  that should still be live.
- `LARGE shrink: free neighbor first`: the original data survives, but the final
  free causes SIGSEGV.

These failures expose implementation defects through regression tests. They are
not skipped or treated as successful outcomes. The allocator implementation has
not been modified as part of this testing work.

## Upcoming Feature — Bonus Part

The following items are reserved for future work and are not tested by this suite:

- pthread-based thread safety and concurrency stress tests.
- Validation of the `MALLOC_DEBUG` and `MALLOC_SCRIBBLE` environment variables.
- Validation of the `show_alloc_mem_ex` hex dump.
- Defragmentation: merging adjacent free blocks and reusing space left by realloc shrinking.

Existing bonus code does not count as verified bonus functionality in this suite.
