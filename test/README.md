# malloc Tests

Run commands from the project root. A C compiler, Linux/POSIX process APIs,
pthread, `make`, and `cp` are required.

## Commands

| Command | Scope |
| --- | --- |
| `make test` or `make test-all` | M1–M5, all edge cases, mandatory regression tests with bonuses enabled, and M1 build/source checks |
| `make test-edge` | 16 mandatory edge cases + 12 M2–M5 edge cases |
| `make test-m1` | 16 mandatory regression cases + 9 M1 cases + 5 build/source checks |
| `make test-m2` | 9 coalescing, shrinking, and heap reclamation cases |
| `make test-m3` | 5 environment variable, logging, scribble, and hex dump cases |
| `make test-m4` | 3 concurrency, pointer transfer, and fault injection cases |
| `make test-m5` | 2 heap reuse and regression cases with bonuses enabled |
| `make test-build` | Build all test executables |
| `make test-correction` | Build and run correction test0–test5 |
| `make test-correction-build` | Build correction executables only |
| `make test-clean` | Remove test object files; keep executables |
| `make test-fclean` | Remove test object files and executables, including test_malloc |

`test-m5` runs only the M5 tests. Use `test-all` for full validation before submission.
Run individual cases using a zero-based index, for example `./test/bin/edge_cases 16`.
Calling an executable without arguments prints its case count.

The correction tests link against the actual shared library. Their executables are
created in `test/bin/correction/`, and their object files in `test/obj/correction/`.
Run individual tests with commands such as `./test/bin/correction/test3`. The
executables locate the project library through their rpath, so no separate
LD_PRELOAD configuration is required. `test-correction` reports PASS/FAIL based on
each program's exit code and continues running the remaining programs after a
failure. The supplied programs are intended for inspecting output and behavior;
they do not automatically validate output contents or memory usage. They are not
included in `test-all`. These builds omit Werror to allow unused-variable warnings
in the original correction sources. The existing structured tests retain Werror.

## Structure

```text
test/
├── correction/       test0.c … test5.c
├── milestones/       m1_cases.c/.h … m5_cases.c/.h
├── all_cases/        all_cases.c/.h
├── helpers/          test_helpers.c/.h, bonus_helpers.c/.h
├── ui/               test_ui.c/.h
├── edge_cases/       edge_cases.c, bonus_edge_cases.c/.h
├── run_tests.c
├── codex.md
├── README.md
├── obj/              correction object files (excluded from Git)
└── bin/              generated executables (excluded from Git)
```

Headers contain only function declarations. C files obtain type and system
declarations through `inc/malloc.h`. New tests and the UI use the project header
and `write`-based output. The existing `run_tests.c` retains the system headers
used for M1's temporary build and source checks.

## Results and Isolation

Each runtime case runs in a separate process. Success is shown as a green `PASS`,
and failure as a red `FAIL`. Assertion failures, crashes, and timeouts all cause
the overall run to fail, while the remaining cases continue running. Executables
return 1 on failure, and `make` exits with a nonzero status on failure. Core dumps
are disabled.

Runtime cases use a 10-second alarm, and the integrated runner uses a 20-second
exec alarm. The dedicated M1 runner uses a 15-second external runtime limit and
a 60-second limit for each build. Output is captured in an unlinked temporary
file. The integrated display shows the first line on success and up to 4095 bytes
on failure. M1 builds run in temporary copies of the project.

Allocator functions are renamed to `edge_malloc`, `edge_free`, and `edge_realloc`
when linked, so libc's internal allocations do not enter the heap under test.
LD_PRELOAD and integration with actual system programs require separate
validation. The integrated runner reads each executable's case count, so adding
cases does not require updating a hardcoded count.

## Edge Cases

The original 16 mandatory cases validate NULL/0 handling, alignment and size
boundaries, overflow, realloc preservation and failure, LARGE allocation lifetime
after shrinking, minimum splitting, fragmentation and multiple heaps, heap
unlinking, and allocation inspection output. malloc(0) and realloc(ptr,0) follow
this project's NULL policy.

The 12 additional M2–M5 cases validate the following:

| Area | Additional cases |
| --- | --- |
| M2 | Exact minimum split remainder, shrinking between live neighbors, reuse of coalesced space |
| M3 | Scribble changes at runtime and empty values, realloc(NULL,0/1), exclusion of freed holes from dumps |
| M4 | Unlocking after 0/NULL paths, first allocation by 8 workers, unlocking and reuse after overflow failure |
| M5 | 100 SMALL heap reuse cycles, reclamation of middle and both outer LARGE heaps, original allocation preservation and recovery after mapping failure |

Split boundaries are checked using metadata constructed in a separate mmap region.
Freed payloads are not read. M4's first-allocation test synchronizes worker starts
with a condition variable; even if pthread_create partially fails, all created
workers are released and joined. RLIMIT_AS fault injection applies only to the
case's process. The original limit is restored before verifying that allocation
succeeds again. Freeing arbitrary pointers and double frees are not covered.

## Milestone Coverage

M1 checks up to 100 blocks, group isolation and fragmentation, memmove, address
ordering and payload boundaries, Total, page size, mapping geometry, and build
contracts. Source checks are limited to the known `d - s` expression and Linux
`getpagesize()` usage.

M2 checks the 128/64/64 shrinking regression, coalescing in both directions and
with both neighbors, recovery of metadata space, and reclamation of multiple
heaps. Shared metadata checks validate links, physical bounds, count, free_size,
and adjacent free blocks. Retaining the last empty TINY/SMALL heap is allowed.

M3 follows the current environment variable interpretation. MALLOC_DEBUG is
enabled whenever it exists, regardless of its value, and is cached after
initialization. MALLOC_SCRIBBLE is refreshed on every call and is enabled when
it exists and its first character is not `0`. An empty value also enables it.
realloc(NULL,n) must initialize the requested bytes to `0xaa`, just like malloc.
Hex dump checks cover 16-byte rows, partial rows, empty output, known live patterns,
and Total. Matching every log argument, full address ordering in extended output,
the tail added by realloc growth, and `0xdd` instrumentation before munmap remain
unverified.

M4 performs 120 allocation, growth, shrink, and free cycles with each of 2, 4,
and 8 workers. A separate mutex synchronizes payload writes and dump reads,
while public allocator calls for independent payloads run concurrently. Metadata
is also checked after join. Pointer ownership is transferred to another thread.
The fault injection check passes only when corruption in all 4 workers propagates
to the final failure result.

M5 checks heap reuse and growth/overflow regressions with debug and scribble
enabled. The integrated run also repeats the 16 mandatory cases with debug and
scribble enabled. Heap reuse is verified through metadata; actual mmap/munmap
system call counts are not measured. Other operating systems remain unverified.

## Local Results

Linux run results from 2026-10-08:

- M1: 30/30 passed.
- M2: 9/9 passed, 0 failures, `make test-m2` exit code 0. Shrinking, consecutive coalescing, coalescing in both directions, metadata, reuse, and heap reclamation checks passed.
- M3: 5/5 passed, 0 failures, `make test-m3` exit code 0. Value interpretation and caching for both variables, logging, scribble, and hex dump checks passed. The existing check for missing scribble in realloc(NULL,n) also passed.
- M4: 2/3 passed. Adjacent free blocks were found after realloc shrinking.
- M5: 2/2 passed.
- Edge cases: 27/28 passed. Scribble was missing in realloc(NULL,1).
- Full integrated run: 82/89 passed, executable exit code 1.

The M2 and M3 results come from individual reruns on 2026-10-08. The other entries
and the full integrated result are records from an earlier run; they have not
been recalculated to reflect the passing M2 and M3 results.

The 89 entries include 25 M1 runtime cases, 19 M2–M5 runtime cases, 28 edge cases,
16 mandatory cases with bonuses enabled, and 1 summary entry that runs the
existing full M1 runner. The last entry counts 30 internal checks as one entry,
so this does not represent 89 independent contracts. Failures are reported as
actual failures. These results do not imply milestone completion or satisfaction
of all submission requirements.
