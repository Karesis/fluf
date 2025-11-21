#pragma once

#include <core/mem/layout.h>
#include <core/mem/allocer.h>
#include <core/type.h>
#include <core/msg.h>

/*
 * ==========================================================================
 * 1. Internal Structures
 * ==========================================================================
 */

/**
 * @brief Footer stored at the end of each memory chunk.
 *
 * Memory Layout:
 * [------------------ data ------------------][-- ChunkFooter --]
 * ^                                          ^                 ^
 * |                                          |                 |
 * data_start                                ptr               data_end
 * (Returned by backing alloc)              (Bump ptr)
 *
 * The bump pointer starts at `data_end - sizeof(ChunkFooter)` and grows down
 * towards `data_start`.
 *
 * @note Why use `u8*`?
 * 1. **Arithmetic**: We perform direct pointer subtraction and alignment calculations
 * on these fields. ISO C forbids arithmetic on `void*` (because size is unknown).
 * Using `u8*` guarantees a stride of exactly 1 byte.
 * 2. **Semantics**: Unlike `char*` (which implies text/strings), `u8*` explicitly
 * signifies "raw memory bytes" treated as unsigned numeric values.
 */
typedef struct ChunkFooter {
	u8 *data_start;
	usize chunk_size;
	struct ChunkFooter *prev;
	u8 *ptr; /// current bump pointer
	usize allocated_bytes; /// for stats
} chunk_footer_t;

/*
 * ==========================================================================
 * 2. The Bump Arena Type
 * ==========================================================================
 */

typedef struct Bump {
	/// The current active chunk where allocations happen.
	chunk_footer_t *current_chunk;

	/// The backing allocator used to request new chunks.
	allocer_t backing;

	/// Allocation limits and settings.
	usize limit;
	usize allocated; // Total bytes allocated from backing allocator
	usize min_align; // Minimum alignment for every alloc
} bump_t;

/*
 * ==========================================================================
 * 3. Lifecycle API (Init/Deinit for Stack, New/Drop for Heap)
 * ==========================================================================
 */

/**
 * @brief Initialize a bump arena on the stack (or pre-allocated memory).
 *
 * @param self    Pointer to the uninitialized bump_t structure.
 * @param backing The allocator to use for getting chunks (e.g., system_allocator()).
 * @param min_align Minimum alignment (must be power of 2). 1 is default.
 */
void bump_init(bump_t *self, allocer_t backing, usize min_align);

/**
 * @brief De-initialize the arena.
 * Frees all chunks allocated by this arena back to the backing allocator.
 * Does NOT free the `bump_t` structure itself.
 */
void bump_deinit(bump_t *self);

/**
 * @brief Allocate a new bump arena on the heap (using the backing allocator).
 *
 * @param backing The allocator used for BOTH the bump_t struct AND its chunks.
 * @return Pointer to the new arena, or nullptr on OOM.
 */
bump_t *bump_new(allocer_t backing, usize min_align);

/**
 * @brief Destroy the arena and free the `bump_t` structure.
 * @note Only use this if the arena was created with `bump_new`.
 */
void bump_drop(bump_t *self);

/**
 * @brief Reset the arena without freeing chunks.
 *
 * Keeps the current chunk active but resets its pointer.
 * Frees all *other* chunks to the backing allocator.
 * Useful for per-frame allocators.
 */
void bump_reset(bump_t *self);

/*
 * ==========================================================================
 * 4. Allocation API (Direct)
 * ==========================================================================
 * ## Design Philosophy: Why raw pointers?
 *
 * * You might wonder why this module returns `anyptr` (void*) instead of a
 * safe `Result<anyptr, error>` type.
 *
 * 1. **Performance**: Bump allocation is designed to be extremely fast (often
 * just a pointer subtraction). Wrapping the return value adds overhead.
 * 2. **Convention**: In low-level C memory management, `NULL` is the
 * universal signal for "Out Of Memory".
 * 3. **Polymorphism**: The `allocer_t` virtual table interface requires
 * returning raw pointers. Keeping the direct API consistent with the
 * vtable interface avoids confusion.
 *
 * @note Always check for `nullptr` if you are not using the `unwrap` macros!
 */

/**
 * @brief Allocate raw memory from the arena.
 */
[[nodiscard]]
anyptr bump_alloc_layout(bump_t *self, layout_t layout);

/**
 * @brief Allocate raw memory (shorthand).
 */
[[nodiscard]]
anyptr bump_alloc(bump_t *self, usize size, usize align);

/**
 * @brief Allocate and zero-initialize.
 */
[[nodiscard]]
anyptr bump_zalloc(bump_t *self, layout_t layout);

/**
 * @brief Allocate and copy data.
 */
[[nodiscard]]
anyptr bump_alloc_copy(bump_t *self, const void *src, usize size, usize align);

/**
 * @brief Allocate and copy a C-string.
 */
[[nodiscard]]
char *bump_alloc_str(bump_t *self, const char *str);

/**
 * @brief Resize a memory block (Pseudo-realloc).
 *
 * Since bump allocators cannot really resize in place (unless it's the last alloc),
 * this typically allocates new memory and copies data.
 */
[[nodiscard]]
anyptr bump_realloc(bump_t *self, anyptr old_ptr, usize old_size,
		    usize new_size, usize align);

/*
 * ==========================================================================
 * 5. Capacity & Inspection
 * ==========================================================================
 */

/**
 * @brief Set a hard limit on total memory usage.
 *
 * @param limit The maximum bytes this arena can allocate from the backing allocator.
 * Pass `SIZE_MAX` to disable the limit.
 */
void bump_set_allocation_limit(bump_t *self, usize limit);

/**
 * @brief Get total bytes currently allocated from the backing allocator.
 *
 * @note This includes internal overhead (chunk footers and alignment padding),
 * so it will be slightly larger than the sum of user allocations.
 *
 * @return Total bytes allocated.
 */
usize bump_get_allocated_bytes(bump_t *self);

/*
 * ==========================================================================
 * 6. Allocer Interface (VTable Adapter)
 * ==========================================================================
 */

/**
 * @brief Convert the bump arena into a generic `allocer_t`.
 * This allows passing the arena to any API expecting an `allocer_t`.
 */
allocer_t bump_allocer(bump_t *self);

/*
 * ==========================================================================
 * 7. Helper Macros (Type-Safe Syntax Sugar)
 * ==========================================================================
 */

#define bump_alloc_type(bump, T) (T *)bump_alloc_layout(bump, layout_of(T))

#define bump_alloc_array(bump, T, count) \
	(T *)bump_alloc_layout(bump, layout_of_array(T, count))

#define bump_zalloc_type(bump, T) (T *)bump_zalloc(bump, layout_of(T))

#define bump_zalloc_array(bump, T, count) \
	(T *)bump_zalloc(bump, layout_of_array(T, count))

/**
 * @brief Alloc a copy of an array.
 */
#define bump_alloc_array_copy(bump, T, src_ptr, count) \
	(T *)bump_alloc_copy(bump, src_ptr, sizeof(T) * (count), alignof(T))
