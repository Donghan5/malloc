# ft_malloc — Architecture

## Overview

`ft_malloc` is a custom dynamic memory allocator implemented as a shared library (`libft_malloc.so`). It replaces the standard `malloc`, `free`, and `realloc` by managing memory directly through `mmap`/`munmap` syscalls, with a three-tier size-class design and a doubly-linked heap/block list.

---

## Size Classes

All allocation requests are classified into one of three groups before any heap is touched:

| Group | Block size range          | Heap size         | Strategy          |
|-------|--------------------------|-------------------|-------------------|
| TINY  | `<= TINY_BLOCK_SIZE`     | `4 × getpagesize()` = 16 KB  | Pre-allocated pool, first-fit split |
| SMALL | `<= SMALL_BLOCK_SIZE`    | `32 × getpagesize()` = 128 KB | Pre-allocated pool, first-fit split |
| LARGE | `> SMALL_BLOCK_SIZE`     | `sizeof(t_heap) + sizeof(t_block) + size` | Exact-fit, one block per heap |

Size thresholds are derived from the heap sizes:

```c
#define TINY_HEAP_ALLOCATION_SIZE  (4  * getpagesize())   // 16 384 B
#define TINY_BLOCK_SIZE            (TINY_HEAP_ALLOCATION_SIZE  / 128)  // 128 B
#define SMALL_HEAP_ALLOCATION_SIZE (32 * getpagesize())   // 131 072 B
#define SMALL_BLOCK_SIZE           (SMALL_HEAP_ALLOCATION_SIZE / 128)  // 1 024 B
```

All requested sizes are rounded up to **16-byte alignment** before classification:

```c
size = (size + 15) & ~15;
```

---

## Memory Layout

Each `mmap` region begins with a `t_heap` header immediately followed by one or more `t_block` headers and their data payloads:

```
┌─────────────────────────────────────────────────────────┐
│  t_heap  (metadata about this mmap region)              │
│  - prev / next  (heap linked list pointers)             │
│  - group        (TINY / SMALL / LARGE)                  │
│  - total_size   (total mmap'd bytes)                    │
│  - free_size    (total bytes in free blocks)            │
│  - block_count  (number of block headers)               │
├─────────────────────────────────────────────────────────┤
│  t_block  (metadata for allocation #1)                  │
│  - prev / next  (block linked list pointers)            │
│  - data_size    (usable bytes in this block)            │
│  - is_free      (true if available for allocation)      │
├─────────────────────────────────────────────────────────┤
│  data payload  [data_size bytes]                        │
├─────────────────────────────────────────────────────────┤
│  t_block  (metadata for allocation #2)                  │
├─────────────────────────────────────────────────────────┤
│  data payload                                           │
│  ...                                                    │
└─────────────────────────────────────────────────────────┘
```

Two macros navigate between the metadata and data layers:

```c
#define HEAP_SHIFT(start)  ((void *)start + sizeof(t_heap))   // heap → first block
#define BLOCK_SHIFT(start) ((void *)start + sizeof(t_block))  // block → data payload
```

---

## Data Structures

### t_heap — heap descriptor

```c
typedef struct s_heap {
    struct s_heap  *prev;
    struct s_heap  *next;
    t_heap_group    group;        // TINY | SMALL | LARGE
    size_t          total_size;   // bytes in the mmap region
    size_t          free_size;    // bytes held by free blocks (header + data)
    size_t          block_count;  // number of t_block entries
} t_heap;
```

All heaps form a **global doubly-linked list** rooted at `g_data.heap_anchor`.

### t_block — block descriptor

```c
typedef struct s_block {
    struct s_block *prev;
    struct s_block *next;
    size_t          data_size;  // usable bytes (excludes this header)
    bool            is_free;
} t_block;
```

Blocks inside a heap form a **doubly-linked list** in address order.

### t_malloc_data — global state wrapper

```c
typedef struct s_malloc_data {
    t_heap  *heap_anchor;       // head of the global heap list
    size_t   tiny_heap_count;   // active TINY heaps
    size_t   small_heap_count;  // active SMALL heaps
} t_malloc_data;
```

Global variables:

```c
t_malloc_data    g_data;               // heap state
pthread_mutex_t  g_malloc_mutex;       // POSIX mutex for thread safety
```

---

## Allocation Flow — `malloc(size)`

```
malloc(size)
  └─ pthread_mutex_lock
  └─ start_malloc(size)
       ├─ align size to 16 bytes
       ├─ classify: get_heap_group_from_block_size(size)
       │
       ├─ [LARGE]
       │    └─ get_heap_of_block_size(size)   → create new heap
       │    └─ append_empty_block(heap, size) → place one block, update free_size
       │    └─ return data pointer
       │
       └─ [TINY / SMALL]
            ├─ find_free_block(&block, size, heap_anchor)
            │    └─ linear scan: heap list → block list
            │    └─ returns first block where is_free && data_size >= size
            │
            ├─ [block found]  ← first-fit path
            │    ├─ split_block(block, size)   → carve remainder as new free block
            │    ├─ block->is_free = false
            │    ├─ heap->free_size -= (size + sizeof(t_block))
            │    └─ return BLOCK_SHIFT(block)
            │
            └─ [no block found]  ← no room in existing heaps
                 ├─ get_heap_of_block_size(size) → create new heap
                 ├─ use first block of new heap
                 ├─ split_block, mark allocated, update free_size
                 └─ return BLOCK_SHIFT(block)
  └─ ft_memset(result, 0xaa, size)   // debug: fill with 0xAA
  └─ pthread_mutex_unlock
```

### Heap creation — `create_new_heap`

- `mmap(MAP_PRIVATE | MAP_ANONYMOUS)` for the full heap size
- For TINY/SMALL: one initial free block spanning the entire usable space is placed immediately; the heap is ready for split-on-demand
- For LARGE: no initial block; `append_empty_block` places the single block

### Block splitting — `split_block`

When a free block is larger than the requested size and the remainder can hold a new block:

```
Before:  [t_block | ........... data_size ........... ]
After:   [t_block | size data ] [t_block | remainder ]
                 allocated ↑         free ↑
```

Condition to split: `block->data_size - size >= sizeof(t_block) + 16`

If the remainder is too small, the block is used as-is (no split).

---

## Deallocation Flow — `free(ptr)`

```
free(ptr)
  └─ pthread_mutex_lock
  └─ start_free(ptr)
       └─ search_pointer(heap, block, ptr)  ← O(n) scan
       └─ block->is_free = true
       │
       ├─ [TINY / SMALL]
       │    ├─ ft_memset(data, 0xdd, data_size)    // debug: poison freed memory
       │    ├─ heap->free_size += (data_size + sizeof(t_block))
       │    ├─ coalesce_block(block)               // merge adjacent free blocks
       │    └─ if heap has only one free block → remove_heap(heap)
       │
       └─ [LARGE]
            ├─ ft_memset(data, 0xdd, data_size)
            ├─ heap->free_size += ...
            ├─ heap->block_count--
            └─ remove_heap(heap)                   // always munmap LARGE heaps
  └─ pthread_mutex_unlock
```

### Block coalescing — `coalesce_block`

After marking a block free, adjacent free blocks are merged to prevent fragmentation:

```
[free A] [free B] [free C]  →  [free A+B+C]
```

- Forward merge: if `block->next->is_free`, absorb it (`A.data_size += sizeof(t_block) + B.data_size`)
- Backward merge: if `block->prev->is_free`, absorb current block into prev

### Heap removal — `remove_heap`

- LARGE heaps: always `munmap`'d on free
- TINY/SMALL heaps: `munmap`'d only when fully coalesced back to a single free block AND a heap of that group still remains (to keep at least one pre-allocated pool alive)

---

## Reallocation Flow — `realloc(ptr, size)`

```
realloc(ptr, size)
  ├─ ptr == NULL  → malloc(size)
  ├─ size == 0    → free(ptr), return NULL
  │
  └─ search_pointer to find block
       ├─ original_size >= size  → return ptr  (shrink: no-op, excess held)
       └─ original_size < size   → grow path:
            ├─ start_malloc(size)
            ├─ ft_memmove(new_ptr, ptr, original_size)
            ├─ start_free(ptr)
            └─ return new_ptr
```

> Note: In-place growth (checking the next free block) is not implemented; every grow always allocates a new region.

---

## Thread Safety

All three public functions (`malloc`, `free`, `realloc`) and the diagnostic `show_alloc_mem` acquire `g_malloc_mutex` (a `PTHREAD_MUTEX_INITIALIZER` global) before touching any shared state and release it on return.

---

## Diagnostic — `show_alloc_mem`

Prints all currently allocated (non-free) blocks grouped by heap type:

```
===== Show Allocated Memory =====
TINY  : 0x7f...
0x7f...a - 0x7f...b : 32 bytes
SMALL : 0x7f...
...
Total : N bytes
=================================
```

---

## File Map

```
inc/
  malloc.h        — master include (pulls struct, define, functions)
  struct.h        — t_heap, t_block, t_malloc_data, t_heap_group
  define.h        — HEAP_SHIFT, BLOCK_SHIFT, size-class macros
  functions.h     — all function prototypes

src/
  malloc.c        — malloc(), start_malloc(), global g_data / mutex
  free.c          — free(), start_free()
  realloc.c       — realloc(), start_realloc()

  block/
    init_block.c  — init_block()
    block.c       — split_block(), coalesce_block(), find_free_block(),
                    remove_block_if_last()

  heap/
    heap.c        — create_new_heap(), append_empty_block(),
                    remove_heap(), is_last_of_preallocated()
    get_heap.c    — get_heap_of_block_size(), get_available_heap(),
                    get_last_heap(), get_last_block()
    helper_heap.c — get_heap_group_from_block_size(),
                    get_heap_size_from_block_size()

  tools/
    pointer.c     — search_pointer()
    show_alloc_mem.c — show_alloc_mem(), print_alloc_mem(),
                       print_heap_header(), print_block_list()
    tools.c       — ft_memset(), ft_memmove(), ft_strlen(),
                    ft_putstr_fd(), ft_print_unsigned_fd(),
                    print_memory_address_portable()

main.c            — test driver (not part of the library)
Makefile          — builds libft_malloc_$(HOSTTYPE).so
```

---

## Build

```sh
make          # builds libft_malloc_$(HOSTTYPE).so + copies to libft_malloc.so
make run      # builds + runs main.c test driver
make valgrind # runs test under valgrind --leak-check=full
make fclean   # removes objects and .so files
```

The library is compiled with `-Wall -Wextra -Werror -fPIC -shared`.
