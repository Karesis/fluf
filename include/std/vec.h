#pragma once

#include <core/mem/allocer.h>
#include <core/msg.h>
#include <core/macros.h>
#include <core/type.h>
#include <stdalign.h> // for alignof

/*
 * ==========================================================================
 * 1. The Generic Type Definition
 * ==========================================================================
 * Usage: 
 * vec(int) numbers;
 * vec(struct Point) points;
 */
#define vec(T)                 \
	struct {               \
		T *data;       \
		usize len;     \
		usize cap;     \
		allocer_t alc; \
	}

#define defVec(type, name) typedef vec(type) name

/*
 * ==========================================================================
 * 2. Public Interface (Type-Safe Macros)
 * ==========================================================================
 */

/**
 * @brief Initialize a vector.
 * @param v The vector variable (passed by value, address taken internally).
 * @param allocator The backing allocator.
 * @param capacity Initial capacity hint (can be 0).
 * @return true on success, false on OOM.
 */
#define vec_init(v, allocator, capacity)                                       \
	_vec_init_impl((anyptr) & (v), allocator, capacity, sizeof(*(v).data), \
		       alignof(typeof(*(v).data)))

/**
 * @brief Free vector memory.
 * @note Does not free the struct itself, only the internal buffer.
 */
#define vec_deinit(v)                                       \
	_vec_deinit_impl((anyptr) & (v), sizeof(*(v).data), \
			 alignof(typeof(*(v).data)))

/**
 * @brief Create a new vector on the heap (or arena).
 *
 * This allocates the vector header struct using the allocator,
 * AND initializes the internal buffer using the same allocator.
 *
 * @param allocator The allocator for both the struct and the buffer.
 * @param T The element type (e.g., int, struct Point).
 * @param capacity Initial capacity hint.
 * @return A strongly-typed pointer to the new vector.
 *
 * @note **Type System Usage Guide**:
 * 1. **Local Variables (Recommended)**: Use `auto`.
 * @code
 * auto v = vec_new(sys, int, 10); // Type is preserved perfectly
 * vec_push(*v, 42);
 * @endcode
 *
 * 2. **Function Arguments / Struct Members**: Use `defVec`.
 * Since `vec(T)` generates a unique anonymous struct every time,
 * you cannot pass `vec(int)` across functions without a typedef.
 * @code
 * defVec(int, IntVec); // Define named type
 * IntVec *v = (IntVec*)vec_new(sys, int, 10); // Cast needed
 * @endcode
 *
 * 3. **Direct Assignment (Warning)**:
 * `vec(int) *v = vec_new(...)` will trigger "incompatible pointer types"
 * warning because the LHS and RHS define two distinct anonymous structs.
 * Use `auto` to avoid this.
 */
#define vec_new(allocator, T, capacity)                                            \
	({                                                                         \
		/* 1. alloc the header struct */                                   \
		layout_t _l_hdr = layout(sizeof(vec(T)), alignof(vec(T)));         \
		vec(T) *_v_ptr = allocer_alloc(allocator, _l_hdr);                 \
                                                                                   \
		if (_v_ptr) {                                                      \
			/* 2. init the buffer */                                   \
			if (!vec_init(*_v_ptr, allocator, capacity)) {             \
				/* OOM during buffer init: free header and fail */ \
				allocer_free(allocator, _v_ptr, _l_hdr);           \
				_v_ptr = nullptr;                                  \
			}                                                          \
		}                                                                  \
		_v_ptr; /* Return typed pointer (for auto) */                      \
	})

/**
 * @brief Destroy a vector created with vec_new.
 *
 * Frees the internal buffer AND the vector struct itself.
 * @param v Pointer to the vector (vec(T)*).
 */
#define vec_drop(v)                                                                         \
	do {                                                                                \
		if (v) {                                                                    \
			/* Capture allocator before deinit wipes it (conceptually) */       \
			/* Note: vec_deinit doesn't wipe .alc usually, but let's be safe */ \
			allocer_t _a = (v)->alc;                                            \
			layout_t _l_hdr =                                                   \
				layout(sizeof(*(v)), alignof(typeof(*(v))));                \
                                                                                            \
			/* 1. Free internal buffer */                                       \
			vec_deinit(*(v));                                                   \
                                                                                            \
			/* 2. Free header struct */                                         \
			allocer_free(_a, v, _l_hdr);                                        \
		}                                                                           \
	} while (0)

/**
 * @brief Reserve capacity for at least `n` additional elements.
 */
#define vec_reserve(v, n)                                         \
	_vec_reserve_impl((anyptr) & (v), (n), sizeof(*(v).data), \
			  alignof(typeof(*(v).data)))

/**
 * @brief Push an element to the end.
 * @return true on success, false on OOM.
 */
#define vec_push(v, val)                                                  \
	({                                                                \
		bool _ok = true;                                          \
		if (unlikely((v).len >= (v).cap)) {                       \
			_ok = _vec_grow_impl((anyptr) & (v),              \
					     sizeof(*(v).data),           \
					     alignof(typeof(*(v).data))); \
		}                                                         \
		if (likely(_ok)) {                                        \
			(v).data[(v).len++] = (val);                      \
		}                                                         \
		_ok;                                                      \
	})

/**
 * @brief Pop an element from the end.
 * @return The element value.
 * @warning Behavior is undefined (crash or garbage) if vector is empty.
 */
#define vec_pop(v) ((v).data[--(v).len])

/**
 * @brief Get a pointer to the last element.
 * @return T* (or undefined if empty).
 */
#define vec_last(v) (&(v).data[(v).len - 1])

/**
 * @brief Safe access with bounds checking.
 * @panic Panics if index is out of bounds.
 */
#define vec_at(v, i)                                                           \
	({                                                                     \
		usize _idx = (i);                                              \
		if (unlikely(_idx >= (v).len)) {                               \
			log_panic("vec index out of bounds: %zu >= %zu", _idx, \
				  (v).len);                                    \
		}                                                              \
		(v).data[_idx];                                                \
	})

/* --- Utilities --- */

#define vec_len(v) ((v).len)
#define vec_cap(v) ((v).cap)
#define vec_is_empty(v) ((v).len == 0)
#define vec_data(v) ((v).data)

#define vec_clear(v) ((v).len = 0)

/**
 * @brief Unsafely set the length. 
 * Useful when manually writing to .data buffer.
 */
#define vec_set_len(v, new_len)                             \
	do {                                                \
		massert((new_len) <= (v).cap,               \
			"vec_set_len: new_len > capacity"); \
		(v).len = (new_len);                        \
	} while (0)

/**
 * @brief Iterate over vector.
 * @param it Iterator name (pointer to element type: T*).
 */
#define vec_foreach(it, v) \
	for (typeof((v).data) it = (v).data; it < (v).data + (v).len; ++it)

/*
 * ==========================================================================
 * 3. Internal Implementation
 * ==========================================================================
 */

[[nodiscard]]
bool _vec_init_impl(anyptr vec, allocer_t alc, usize cap, usize item_size,
		    usize align);
void _vec_deinit_impl(anyptr vec, usize item_size, usize align);
[[nodiscard]]
bool _vec_grow_impl(anyptr vec, usize item_size, usize align);
[[nodiscard]]
bool _vec_reserve_impl(anyptr vec, usize additional, usize item_size,
		       usize align);
