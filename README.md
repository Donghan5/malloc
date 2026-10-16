# ft_malloc

A custom implementation of `malloc`, `free`, and `realloc` in C, built as a shared library. This project reimplements the standard memory allocator using `mmap`/`munmap` with a three-tier allocation strategy optimized for different block sizes.

## Memory Layout

![Memory Layout](ft_malloc_memory_layout.svg)

Each `mmap` region begins with a `t_heap` metadata header (48 bytes), followed by a doubly-linked list of `t_block` nodes (32 bytes each) and their associated data payloads. The `HEAP_SHIFT` and `BLOCK_SHIFT` macros skip past these headers to return a clean data pointer to the caller.

## Allocation Flow

![Allocation and Deallocation Flow](ft_malloc_allocation_flow.svg)

### malloc

1. Align the requested size to 16 bytes.
2. Classify the request into a size group (TINY / SMALL / LARGE).
3. **LARGE**: `mmap` an independent, page-rounded region including metadata and return the data pointer.
4. **TINY / SMALL**: scan existing heaps with `find_free_block` (first-fit).
   - Block found → `split_block` + mark used → return pointer.
   - No block found → allocate a new heap with `mmap`, split the first block.

### free

- Locate the owning heap and block via `search_pointer`.
- **TINY / SMALL**: mark free, `coalesce_block` (merge adjacent free blocks), release an empty heap with `munmap` unless it is the last heap of its size class. The last TINY heap and last SMALL heap are retained even when empty.
- **LARGE**: `munmap` immediately (scribble `0xdd` over data first if `MALLOC_SCRIBBLE=1`).

### realloc

- `ptr == NULL` → behaves as `malloc(size)`.
- `size == 0` → behaves as `free(ptr)`.
- `original_size >= size` → shrink in place via `split_block` for TINY/SMALL when enough space remains. LARGE returns the same pointer without splitting or reducing its mapping.
- Otherwise → `malloc` new region, `memmove` data, `free` old region.

---

## Size Classes

| Class | Block size | Heap size |
|-------|-----------|-----------|
| TINY  | ≤ 128 B   | 16 KB (4 × page) |
| SMALL | ≤ 1024 B  | 128 KB (32 × page) |
| LARGE | > 1024 B  | page-rounded request + metadata |

The 128/1024-byte boundaries and heap sizes above assume a 4096-byte page.
Sizes are derived at runtime by `get_page_size()` using Linux
`sysconf(_SC_PAGESIZE)`. Successful results are cached in `g_data.page_size`;
failed queries return zero without caching. The macros in `inc/define.h` are:

```c
#define TINY_HEAP_ALLOCATION_SIZE  ((size_t)4 * get_page_size())
#define TINY_BLOCK_SIZE            (TINY_HEAP_ALLOCATION_SIZE / 128)
#define SMALL_HEAP_ALLOCATION_SIZE ((size_t)32 * get_page_size())
#define SMALL_BLOCK_SIZE           (SMALL_HEAP_ALLOCATION_SIZE / 128)
```

---

## Project Structure

```
malloc/
├── inc/
│   ├── malloc.h        # Main header (global vars, includes)
│   ├── struct.h        # t_heap, t_block, t_malloc_data, t_heap_group
│   ├── define.h        # HEAP_SHIFT, BLOCK_SHIFT, size macros
│   └── functions.h     # All function prototypes
├── src/
│   ├── malloc.c        # malloc() + start_malloc()
│   ├── free.c          # free()  + start_free()
│   ├── realloc.c       # realloc() + start_realloc()
│   ├── block/
│   │   ├── block.c     # find_free_block, split_block, coalesce_block
│   │   └── init_block.c
│   ├── heap/
│   │   ├── heap.c      # create_new_heap, remove_heap, append_empty_block
│   │   ├── get_heap.c  # get_heap_of_block_size, get_last_heap, get_last_block
│   │   └── helper_heap.c # get_page_size, group selection and heap size calculation
│   └── tools/
│       ├── show_alloc_mem.c  # show_alloc_mem, show_alloc_mem_ex
│       ├── tools.c           # ft_memset, ft_memmove, ft_putstr_fd, print_memory_address_portable, …
│       └── pointer.c         # search_pointer
└── test/               # Linux evaluation runner and official attachments
```

---

## Build

```bash
make          # builds libft_malloc_<arch>_<OS>.so + symlink libft_malloc.so
make clean    # remove object files
make fclean   # remove allocator objects and library
make re       # finish fclean, then recursively run all (also with -j)
```

The library name embeds the host type automatically:

```makefile
ifeq ($(strip $(HOSTTYPE)),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif
NAME = libft_malloc_$(HOSTTYPE).so
```

### Linux tests

```bash
make help
make test0       # system/user baseline
make test1       # allocation overhead and score interpretation
make test2       # free function and quality
make test3       # realloc
make test4       # show_alloc_mem
make test5       # alignment
make test-edge   # additional overflow checks
make test-bonus  # representative bonus checks
make test        # full evaluation, common prerequisites run once
make check       # runner fixtures only
```

Each command builds in a new isolated workspace and collects its own required
measurements. A repository lock serializes measurements even with `make -j`.
See [test/README.md](test/README.md) for IDs, source conflicts, scoring and exit codes.

---

## Environment Variables

| Variable          | Effect |
|-------------------|--------|
| `MALLOC_DEBUG=1`  | Print each `malloc`, `free`, `realloc` call with address and size |
| `MALLOC_SCRIBBLE=1` | Fill allocated memory with `0xaa`; fill freed LARGE blocks with `0xdd` |

### Bonus: usage of `getenv()`

- **Purpose:** Read `MALLOC_DEBUG` and `MALLOC_SCRIBBLE` to implement the bonus debug environment variables.
- **Value interpretation:** Only the exact string `"1"` enables a flag. Every other value, including an unset variable, `"0"`, an empty string, and `"10"`, disables it.
- **Initialization:** The first execution of `init_debug_flags()` reads both variables and caches their flags. Later environment changes do not update either flag.
- **Basis for use:** The project PDF allows additional functions for bonus implementations and requires explaining their use during evaluation. `getenv()` provides the environment-variable lookup needed for this debug bonus.

Documenting a function does not by itself make arbitrary functions permissible. The justification here connects `getenv()` specifically to the environment-variable debug bonus it implements.

---

## Diagnostics

```c
void show_alloc_mem(void);     // List all live allocations with addresses and sizes
void show_alloc_mem_ex(void);  // Same, plus a hex dump of each block's content
```

Example output:

```
===== Show Allocated Memory =====
TINY : 0x7f3a40000000
0x7f3a40000070 - 0x7f3a40000080 : 16 bytes
Total : 16 bytes
=================================
```

---

## Thread Safety

All public entry points (`malloc`, `free`, `realloc`, `show_alloc_mem`, `show_alloc_mem_ex`) acquire `g_malloc_mutex` (a `pthread_mutex_t`) before touching shared state and release it on exit, making the allocator safe for multithreaded programs.

---

## Global State

```c
t_malloc_data   g_data;          // heap_anchor, heap counts, debug/scribble flags
pthread_mutex_t g_malloc_mutex;  // protects all allocator state
```

---

## Free Fragmentation (bonus)
Coalescing of free blocks is implemented in `coalesce_block()`, which merges adjacent free blocks into a single larger block. This existing implementation makes adjacent free space available as one block for later allocations.

---

## Calloc (bonus)
`calloc` is implemented as a wrapper around `malloc` that multiplies the requested number of elements by the size of each element, allocates the memory, and then zeroes it out using `ft_memset`. This ensures that the allocated memory is initialized to zero.

---
## Author

donghank — 42 School project
