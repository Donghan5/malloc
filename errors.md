# ft_malloc — Bug Audit (2026-04-05)

## Build Error

### 1. Misleading indentation in `remove_heap` ~~--- fix it (check it later)~~ ✅ FIXED (2026-04-06)
- **File:** `src/heap/heap.c:110-113`
- **Severity:** Build-breaking (`-Werror=misleading-indentation`)
- **Description:** The second `if (is_last_of_preallocated(...))` is indented as if guarded by the `if (heap->next)` above it, but it is not. GCC treats this as an error with `-Werror`.
- **Fix:** Align the indentation of the second `if` to the same level as the first `if`.
- **Verified:** Both `if (heap->next)` (line 110) and `if (is_last_of_preallocated(heap))` (line 113) are now at the same indentation level.

---

## Critical Bugs

### 2. `free_size` never updated for TINY/SMALL heaps ~~-- fix it (check it later)~~ ✅ FIXED (2026-04-06) — minor residual issue remains
- **File:** `src/heap/heap.c:73-86` (create_new_heap), `src/malloc.c` (start_malloc), `src/free.c` (start_free)
- **Severity:** Critical
- **Description:** `create_new_heap` sets `free_size = heap_size - sizeof(t_heap)`, then creates an initial block that consumes all of that space but **never subtracts** the block size from `free_size`. Neither `start_malloc` nor `start_free` update `free_size` for TINY/SMALL heaps. As a result, `get_available_heap` always considers TINY/SMALL heaps as having free space, even when they are completely full.
- **Fix:** Subtract `sizeof(t_block) + data_size` from `free_size` after creating the initial block. Update `free_size` on every allocation and free for TINY/SMALL heaps.
- **Verified:**
  - `create_new_heap`: leaving `free_size = heap_size - sizeof(t_heap)` is semantically correct — the initial block is free, so `free_size` correctly reflects full available space.
  - `start_malloc`: now decrements `heap->free_size -= (size + sizeof(t_block))` on both TINY/SMALL allocation paths (lines 56, 67). ✓
  - `start_free`: now increments `heap->free_size += (block->data_size + sizeof(t_block))` for TINY/SMALL (line 36). ✓
- **Residual issue (minor):** When `split_block` cannot split (exact-fit case), `block->data_size` stays at `original_data_size > size`, but `start_malloc` subtracts only `size + sizeof(t_block)` instead of `block->data_size + sizeof(t_block)`. This causes `free_size` to drift upward over repeated no-split alloc/free cycles. It won't cause memory corruption (no free block exists to match the phantom `free_size`), but may cause unnecessary heap creation. Proper fix: use `block->data_size + sizeof(t_block)` after the `split_block` call.

### 3. `start_malloc` second path corrupts memory ✅ FIXED (2026-04-07) — minor inefficiency remains
- **File:** `src/malloc.c:61-73`
- **Severity:** Critical (memory corruption)
- **Description:** When `find_free_block` fails and `get_heap_of_block_size` returns an **existing** heap (due to stale `free_size` from bug #2), the code does `block = (t_block *)HEAP_SHIFT(heap)` and uses the first block blindly. That first block may already be allocated, leading to memory corruption when `split_block` and `is_free = false` are applied to it.
- **Fix:** This path should only operate on newly created heaps, or should search for a free block within the returned heap. Consider restructuring the logic so that `get_heap_of_block_size` only returns heaps that truly have free blocks.
- **Verified:**
  - Guard `if (!block->is_free || block->data_size < size) return (NULL)` at `src/malloc.c:66-67` prevents both corruption (allocated block) and undersized allocation. ✓
  - In practice, this path only succeeds when `get_heap_of_block_size` creates a **new** heap (first block is always free and large enough). If it returns an existing heap, `find_free_block` already failed to find a match, so the guard safely rejects it. ✓
- **Residual note (minor, not a safety issue):** The path only checks the first block of the heap. If `free_size` drifts (bug #2 residual), `get_available_heap` may return an existing heap with free space in non-first blocks, and this path returns NULL instead of finding them. This is a missed allocation, not corruption.

### 4. TINY/SMALL heaps are never freed (memory leak) ✅ FIXED (2026-04-07)
- **File:** `src/free.c:37-47`, `src/heap/heap.c:101-104`
- **Severity:** Critical (memory leak)
- **Description:** Two compounding issues:
  - `block_count` is initialized to 1 in `create_new_heap` and was **never decremented** for TINY/SMALL heaps (not on free, not on coalesce).
  - `remove_heap` checks `if (heap->block_count > 0 && heap->group != LARGE) return;` and always returned early for TINY/SMALL because `block_count` was always >= 1.
  - Even when all blocks were freed and coalesced into a single free block, the heap was never actually munmapped.
- **Fix:** Either maintain `block_count` accurately (decrement on coalesce, increment on split), or change the `remove_heap` guard to check the block list state instead of `block_count`.
- **Verified:**
  - `start_free` (`src/free.c:37-40`): now decrements `block_count` before coalescing — once if `block->next` is free, once if `block->prev` is free. ✓
  - `start_free` (`src/free.c:42-47`): after coalescing, checks if the heap has a single free block (`first->prev == NULL && first->next == NULL && first->is_free == true`), sets `block_count = 0`, and calls `remove_heap`. ✓
  - `remove_heap` guard (`src/heap/heap.c:103`) now passes because `block_count == 0`, allowing the heap to be munmapped. ✓
- **Note:** `block_count` is now fully maintained — see bug #5.

### 5. `block_count` never maintained for TINY/SMALL ✅ FIXED (2026-04-07)
- **File:** `src/block/block.c:58-112` (split_block, coalesce_block), `src/malloc.c:55-70`, `src/free.c:37-47`
- **Severity:** Critical (related to bug #4)
- **Description:** `coalesce_block` merges blocks but never decremented `heap->block_count`. `split_block` creates a new block but never incremented `heap->block_count`. After the first operation, the counter was meaningless.
- **Fix:** Pass the `t_heap *` to `split_block` and `coalesce_block` (or look it up) and update `block_count` on every split (+1) and coalesce (-1).
- **Verified:** Instead of modifying `split_block`/`coalesce_block` directly, `block_count` is now maintained at the call sites:
  - `start_malloc` (`src/malloc.c:57,70`): increments `heap->block_count++` when `split_block` succeeds (returns non-NULL). ✓
  - `start_free` (`src/free.c:37-40`): decrements `heap->block_count--` for each adjacent free block before coalescing. ✓
  - `start_free` (`src/free.c:45`): resets `block_count = 0` when heap is fully coalesced into a single free block. ✓

---

## Logic Bugs

### 6. `split_block` prints error message on normal operation ✅ FIXED (2026-04-07)
- **File:** `src/block/block.c:60-61`
- **Severity:** Medium
- **Description:** When a block is an exact fit and cannot be split (remaining space < `sizeof(t_block) + 16`), the function printed `[MALLOC] Cannot split block` to stdout. This is normal behavior (not an error) and polluted program output.
- **Fix:** Remove the print or change it to a debug-only message.
- **Verified:** Print statement removed. `split_block` now silently returns `NULL` on exact-fit. ✓

### 7. `get_heap_group_from_block_size` returns wrong type ✅ FIXED (2026-04-07)
- **File:** `src/heap/helper_heap.c:18`, `inc/functions.h:26`
- **Severity:** Low
- **Description:** The function returned enum values (`TINY`, `SMALL`, `LARGE`) but was declared with return type `size_t` instead of `t_heap_group`.
- **Fix:** Change the return type to `t_heap_group` in both the definition and declaration.
- **Verified:** Return type is `t_heap_group` in both `helper_heap.c:18` and `functions.h:26`. ✓

### 8. `struct.h` uses `bool` without including `<stdbool.h>` ✅ FIXED (2026-04-07)
- **File:** `inc/struct.h:17`
- **Severity:** Low
- **Description:** `t_block` uses `bool is_free` but `struct.h` only included `<stddef.h>`. It worked because `malloc.h` includes `<stdbool.h>` before including `struct.h`, but would break if `struct.h` were ever included directly.
- **Fix:** Add `#include <stdbool.h>` to `struct.h`.
- **Verified:** `struct.h:17` now includes `#include <stdbool.h>`. ✓

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
| **P0 — Fix first** | ~~#1~~ ✅ | Build fixed |
| **P1 — Fix next** | ~~#2~~ ✅, ~~#3~~ ✅, ~~#4~~ ✅, ~~#5~~ ✅ | All critical bugs fixed; #2 and #3 have minor residual notes |
| **P2 — Should fix** | ~~#6~~ ✅, ~~#7~~ ✅, #9 | #9 still open (realloc shrink doesn't split) |
| **P3 — Nice to fix** | ~~#8~~ ✅, #10, #11, #12, #13 | #10-#13 still open (code quality) |
