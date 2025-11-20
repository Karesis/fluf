#pragma once

#include <core/math.h> /// for is_power_of_two
#include <core/msg.h> /// for massert
#include <core/type.h>

#include <stddef.h> /// for size_t
#include <stdalign.h> /// for alignof

/**
 * @brief A memory layout descripter
 *
 * Describes the size and alignment requirements for a block of memory.
 * Just like `std::alloc::Layout` in Rust.
 */
typedef struct Layout {
	usize size;
	usize align;
} layout_t;

/**
 * @brief Create a `Layout` instance.
 * (`Layout::from_size_align`)
 *
 * @param size The size of the memory (byte).
 * @param align The alignment of the memory (in bytes). Must be a power of two.
 * @panic Triggers the assertion (in debug mode) if `align` is not a power of two.
 */
static inline layout_t layout(usize size, usize align)
{
	massert(is_power_of_two(align),
		"Layout alignment must be a power of two");
	return (layout_t){ .size = size, .align = align };
}

/**
 * @brief Get the layout of a single type T.
 * (`Layout::new::<T>`)
 *
 * @param T The type of the item.
 * @return The layout description of the item.
 */
#define layout_of(T) (layout(sizeof(T), alignof(T)))

/**
 * @brief Creates a layout for an array of `N` items of type `T`.
 * (`Layout::array::<T>(N)`)
 *
 * @param T Type of the item.
 * @param N The number of items in the array.
 * @return The layout.
 */
#define layout_of_array(T, N) (layout(sizeof(T) * (N), alignof(T)))
