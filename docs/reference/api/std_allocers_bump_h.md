# std/allocers/bump.h

## `typedef struct ChunkFooter {`


Footer stored at the end of each memory chunk.

Memory Layout:
[------------------ data ------------------][-- ChunkFooter --]
^                                          ^                 ^
|                                          |                 |
data_start                                ptr               data_end
(Returned by backing alloc)              (Bump ptr)

The bump pointer starts at `data_end - sizeof(ChunkFooter)` and grows down
towards `data_start`.


> **Note:** Why use `u8*`?
1. **Arithmetic**: We perform direct pointer subtraction and alignment calculations
on these fields. ISO C forbids arithmetic on `void*` (because size is unknown).
Using `u8*` guarantees a stride of exactly 1 byte.
2. **Semantics**: Unlike `char*` (which implies text/strings), `u8*` explicitly
signifies "raw memory bytes" treated as unsigned numeric values.


---

## `void bump_init(bump_t *self, allocer_t backing, usize min_align);`


Initialize a bump arena on the stack (or pre-allocated memory).


- **`self`**: Pointer to the uninitialized bump_t structure.
- **`backing`**: The allocator to use for getting chunks (e.g., system_allocator()).
- **`min_align`**: Minimum alignment (must be power of 2). 1 is default.


---

## `void bump_deinit(bump_t *self);`


De-initialize the arena.
Frees all chunks allocated by this arena back to the backing allocator.
Does NOT free the `bump_t` structure itself.


---

## `bump_t *bump_new(allocer_t backing, usize min_align);`


Allocate a new bump arena on the heap (using the backing allocator).


- **`backing`**: The allocator used for BOTH the bump_t struct AND its chunks.
- **Returns**: Pointer to the new arena, or nullptr on OOM.


---

## `void bump_drop(bump_t *self);`


Destroy the arena and free the `bump_t` structure.

> **Note:** Only use this if the arena was created with `bump_new`.


---

## `void bump_reset(bump_t *self);`


Reset the arena without freeing chunks.

### Strategy: "Keep the Tip".
This function frees all chunks *except* the current one.
Frees all *other* chunks to the backing allocator,
useful for per-frame allocators.

### Why?
Since chunks typically grow geometrically (doubling size), the current chunk
is usually large enough to hold the entire working set of the next cycle
in a single contiguous block. This reduces fragmentation and malloc calls
for subsequent runs, while still releasing excess memory from peak spikes.


---

## `[[nodiscard]] anyptr bump_alloc_layout(bump_t *self, layout_t layout);`


Allocate raw memory from the arena.


---

## `[[nodiscard]] anyptr bump_alloc(bump_t *self, usize size, usize align);`


Allocate raw memory (shorthand).


---

## `[[nodiscard]] anyptr bump_zalloc(bump_t *self, layout_t layout);`


Allocate and zero-initialize.


---

## `[[nodiscard]] anyptr bump_alloc_copy(bump_t *self, const void *src, usize size, usize align);`


Allocate and copy data.


---

## `[[nodiscard]] char *bump_alloc_cstr(bump_t *self, const char *str);`


Allocate and copy a C-string (const char*).

> **Note:** Performs strlen(). Safe only for null-terminated strings.


---

## `[[nodiscard]] char *bump_dup_str(bump_t *self, str_t s);`


Allocate and copy a string slice, ensuring null-termination.

Creates a stable, null-terminated C-string in the arena from a slice.
Useful for interning or converting slices to C-compatible strings.


> **Note:** O(1) length check (uses slice.len), safer than cstr version.


---

## `[[nodiscard]] anyptr bump_realloc(bump_t *self, anyptr old_ptr, usize old_size, usize new_size, usize align);`


Resize a memory block (Pseudo-realloc).

Since bump allocators cannot really resize in place (unless it's the last alloc),
this typically allocates new memory and copies data.


---

## `void bump_set_allocation_limit(bump_t *self, usize limit);`


Set a hard limit on total memory usage.


- **`limit`**: The maximum bytes this arena can allocate from the backing allocator.
Pass `SIZE_MAX` to disable the limit.


---

## `usize bump_get_allocated_bytes(bump_t *self);`


Get total bytes currently allocated from the backing allocator.


> **Note:** This includes internal overhead (chunk footers and alignment padding),
so it will be slightly larger than the sum of user allocations.


- **Returns**: Total bytes allocated.


---

## `allocer_t bump_allocer(bump_t *self);`


Convert the bump arena into a generic `allocer_t`.
This allows passing the arena to any API expecting an `allocer_t`.


---

