/*
 *    Copyright 2025 Karesis
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <core/mem/allocer.h>
#include <core/msg.h>
#include <core/type.h>
#include <core/macros.h>
#include <core/hash.h>
#include <string.h>

/*
 * ==========================================================================
 * 1. Map Operations (VTable)
 * ==========================================================================
 */

typedef struct {
	/** Hash function: returns a 64-bit hash for the key. */
	u64 (*hash)(const void *key);

	/** Equality function: returns true if two keys are equal. */
	bool (*equals)(const void *key_lhs, const void *key_rhs);
} map_ops_t;

/* --- Pre-defined Operations --- */

/**
 * Ops for specific integer types.
 * Choose based on the sizeof(Key).
 */
extern const map_ops_t MAP_OPS_U32;
extern const map_ops_t MAP_OPS_U64;
extern const map_ops_t MAP_OPS_USIZE; /// Typically matches pointers

/**
 * Ops for pointers (hashes the address itself).
 */
extern const map_ops_t MAP_OPS_PTR;

/**
 * Ops for C-strings (const char*). Hashes the content.
 */
extern const map_ops_t MAP_OPS_CSTR;

/*
 * ==========================================================================
 * 2. Generic Type Definition
 * ==========================================================================
 */

/// Control byte states (Internal)
#define _MAP_EMPTY 0
#define _MAP_FULL 1
#define _MAP_TOMB 2

#define map(K, V)                                                              \
	struct {                                                               \
		K *keys;                                                       \
		V *vals;                                                       \
		u8 *states; /* Metadata: 0=Empty, 1=Full, 2=Deleted */         \
		usize len;                                                     \
		usize cap;                                                     \
		usize occupied; /* len + tombstones (for load factor check) */ \
		allocer_t alc;                                                 \
		map_ops_t ops;                                                 \
		usize key_size; /* Cached for void* impl */                    \
		usize val_size; /* Cached for void* impl */                    \
	}

#define defMap(K, V, Name) typedef map(K, V) Name

/*
 * ==========================================================================
 * 3. Public Interface
 * ==========================================================================
 */

/**
 * @brief Initialize a map.
 * @param m The map variable.
 * @param allocator Backing allocator.
 * @param ops_vtable Operations for Key (hash/eq). Use constants like MAP_OPS_INT.
 */
#define map_init(m, allocator, ops_vtable)                    \
	_map_init_impl((anyptr) & (m), allocator, ops_vtable, \
		       sizeof(*(m).keys), sizeof(*(m).vals))

/**
 * @brief Free map memory.
 */
#define map_deinit(m) \
	_map_deinit_impl((anyptr) & (m), sizeof(*(m).keys), sizeof(*(m).vals))

/**
 * @brief Insert or update a value.
 * @param key Passed by value (will be copied into map).
 * @param val Passed by value.
 * @return true on success, false on OOM.
 */
#define map_put(m, key, val)                             \
	({                                               \
		typeof((m).keys[0]) _k = (key);          \
		typeof((m).vals[0]) _v = (val);          \
		_map_put_impl((anyptr) & (m), &_k, &_v); \
	})

/**
 * @brief Get a pointer to the value.
 * @return Pointer to value, or nullptr if not found.
 * Allows modification: *ptr = new_val;
 */
#define map_get(m, key)                                                      \
	({                                                                   \
		typeof((m).keys[0]) _k_lookup = (key);                       \
		(typeof((m).vals))_map_get_impl((anyptr) & (m), &_k_lookup); \
	})

/**
 * @brief Remove a key.
 * @return true if key existed and was removed.
 */
#define map_remove(m, key)                                 \
	({                                                 \
		typeof((m).keys[0]) _k_del = (key);        \
		_map_remove_impl((anyptr) & (m), &_k_del); \
	})

/**
 * @brief Clear all entries (keeps capacity).
 */
#define map_clear(m) _map_clear_impl((anyptr) & (m))

/* --- Utilities --- */
#define map_len(m) ((m).len)
#define map_cap(m) ((m).cap)

/*
 * ==========================================================================
 * 4. Internals
 * ==========================================================================
 */

[[nodiscard]] bool _map_init_impl(anyptr map, allocer_t alc, map_ops_t ops,
				  usize k_sz, usize v_sz);
void _map_deinit_impl(anyptr map, usize k_sz, usize v_sz);
[[nodiscard]] bool _map_put_impl(anyptr map, const void *k_ptr,
				 const void *v_ptr);
void *_map_get_impl(anyptr map, const void *k_ptr);
bool _map_remove_impl(anyptr map, const void *k_ptr);
void _map_clear_impl(anyptr map);
