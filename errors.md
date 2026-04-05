# ft_malloc — Bug Audit (2026-04-05)

## Build Error

### 1. Misleading indentation in `remove_heap`
- **File:** `src/heap/heap.c:110-113`
- **Severity:** Build-breaking (`-Werror=misleading-indentation`)
- **Description:** The second `if (is_last_of_preallocated(...))` is indented as if guarded by the `if (heap->next)` above it, but it is not. GCC treats this as an error with `-Werror`.
- **Fix:** Align the indentation of the second `if` to the same level as the first `if`.

---

## Critical Bugs

### 2. `free_size` never updated for TINY/SMALL heaps
- **File:** `src/heap/heap.c:73-86` (create_new_heap), `src/malloc.c` (start_malloc), `src/free.c` (start_free)
- **Severity:** Critical
- **Description:** `create_new_heap` sets `free_size = heap_size - sizeof(t_heap)`, then creates an initial block that consumes all of that space but **never subtracts** the block size from `free_size`. Neither `start_malloc` nor `start_free` update `free_size` for TINY/SMALL heaps. As a result, `get_available_heap` always considers TINY/SMALL heaps as having free space, even when they are completely full.
- **Fix:** Subtract `sizeof(t_block) + data_size` from `free_size` after creating the initial block. Update `free_size` on every allocation and free for TINY/SMALL heaps.

### 3. `start_malloc` second path corrupts memory
- **File:** `src/malloc.c:59-67`
- **Severity:** Critical (memory corruption)
- **Description:** When `find_free_block` fails and `get_heap_of_block_size` returns an **existing** heap (due to stale `free_size` from bug #2), the code does `block = (t_block *)HEAP_SHIFT(heap)` and uses the first block blindly. That first block may already be allocated, leading to memory corruption when `split_block` and `is_free = false` are applied to it.
- **Fix:** This path should only operate on newly created heaps, or should search for a free block within the returned heap. Consider restructuring the logic so that `get_heap_of_block_size` only returns heaps that truly have free blocks.

### 4. TINY/SMALL heaps are never freed (memory leak)
- **File:** `src/free.c:37-40`, `src/heap/heap.c:101-104`
- **Severity:** Critical (memory leak)
- **Description:** Two compounding issues:
  - `block_count` is initialized to 1 in `create_new_heap` and is **never decremented** for TINY/SMALL heaps (not on free, not on coalesce).
  - `remove_heap` checks `if (heap->block_count > 0 && heap->group != LARGE) return;` and always returns early for TINY/SMALL because `block_count` is always >= 1.
  - Even when all blocks are freed and coalesced into a single free block, the heap is never actually munmapped.
- **Fix:** Either maintain `block_count` accurately (decrement on coalesce, increment on split), or change the `remove_heap` guard to check the block list state instead of `block_count`.

### 5. `block_count` never maintained for TINY/SMALL
- **File:** `src/block/block.c:58-112` (split_block, coalesce_block)
- **Severity:** Critical (related to bug #4)
- **Description:** `coalesce_block` merges blocks but never decrements `heap->block_count`. `split_block` creates a new block but never increments `heap->block_count`. After the first operation, the counter is meaningless.
- **Fix:** Pass the `t_heap *` to `split_block` and `coalesce_block` (or look it up) and update `block_count` on every split (+1) and coalesce (-1).

---

## Logic Bugs

### 6. `split_block` prints error message on normal operation
- **File:** `src/block/block.c:62`
- **Severity:** Medium
- **Description:** When a block is an exact fit and cannot be split (remaining space < `sizeof(t_block) + 16`), the function prints `[MALLOC] Cannot split block` to stdout. This is normal behavior (not an error) and pollutes program output.
- **Fix:** Remove the print or change it to a debug-only message.

### 7. `get_heap_group_from_block_size` returns wrong type
- **File:** `src/heap/helper_heap.c:18`, `inc/functions.h:26`
- **Severity:** Low
- **Description:** The function returns enum values (`TINY`, `SMALL`, `LARGE`) but is declared with return type `size_t` instead of `t_heap_group`.
- **Fix:** Change the return type to `t_heap_group` in both the definition and declaration.

### 8. `struct.h` uses `bool` without including `<stdbool.h>`
- **File:** `inc/struct.h:54`
- **Severity:** Low
- **Description:** `t_block` uses `bool is_free` but `struct.h` only includes `<stddef.h>`. It works because `malloc.h` includes `<stdbool.h>` before including `struct.h`, but breaks if `struct.h` is ever included directly.
- **Fix:** Add `#include <stdbool.h>` to `struct.h`.

### 9. `realloc` shrink doesn't split block
- **File:** `src/realloc.c:43-44`
- **Severity:** Medium
- **Description:** When `original_size >= size` (shrink case), `start_realloc` returns the original pointer without splitting the block. The excess memory is wasted and cannot be reused by future allocations.
- **Fix:** Call `split_block` when the difference is large enough to create a new free block.

### 10. `ft_memset` in `free.c` runs before coalesce
- **File:** `src/free.c:35`
- **Severity:** Low
- **Description:** `ft_memset(BLOCK_SHIFT(block), 0xdd, block->data_size)` runs before `coalesce_block`. After coalescing, the absorbed block's metadata region becomes part of the data area but was never zeroed with `0xdd`.
- **Fix:** Move `ft_memset` after `coalesce_block` to cover the full coalesced region, or accept the inconsistency.

---

## Code Quality Issues

### 11. Include paths are fragile for `src/tools/` files
- **Files:** `src/tools/show_alloc_mem.c:19`, `src/tools/tools.c:13`
- **Severity:** Low
- **Description:** These files use `#include "../inc/malloc.h"` which resolves to `src/inc/malloc.h` (does not exist). It only compiles because GCC falls back to the `-I inc` search path. Other files in `src/tools/` (like `pointer.c`) correctly use `#include "../../inc/malloc.h"`.
- **Fix:** Change to `#include "../../inc/malloc.h"` or `#include "malloc.h"`.

### 12. `main.c` has wrong include path
- **File:** `main.c:13`
- **Severity:** Low
- **Description:** `#include "../inc/malloc.h"` from the project root resolves to `../inc/malloc.h` (outside the project). Works only via the `-I inc` fallback.
- **Fix:** Change to `#include "malloc.h"` or `#include "inc/malloc.h"`.

### 13. Makefile symlink commented out
- **File:** `Makefile:44-45`
- **Severity:** Low
- **Description:** The `ln -s` is commented out and replaced with `cp`. The `.so` file is duplicated instead of symlinked. If the library is rebuilt, the copy becomes stale until the next `make`.
- **Fix:** Uncomment the `ln -s` line and remove the `cp`.

---

## Fix Priority

| Priority | Bugs | Notes |
|----------|------|-------|
| **P0 — Fix first** | #1 | Build is broken, nothing else can be tested |
| **P1 — Fix next** | #2, #3, #4, #5 | All interrelated: free_size tracking + block_count maintenance |
| **P2 — Should fix** | #6, #7, #9 | Functional issues affecting correctness or usability |
| **P3 — Nice to fix** | #8, #10, #11, #12, #13 | Code quality and robustness |
