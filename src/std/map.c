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

#include <std/map.h>
#include <core/math.h> /// for checked_mul, etc. (if used) or just logic

/*
 * Internal Header Layout
 */
typedef struct {
	u8 *keys;
	u8 *vals;
	u8 *states;
	usize len;
	usize cap;
	usize occupied; /// len + deleted
	allocer_t alc;
	map_ops_t ops;
	usize key_size;
	usize val_size;
} map_header_t;

#define MAX_LOAD_FACTOR_PERCENT 75

/*
 * ==========================================================================
 * Standard Operations Implementation
 * ==========================================================================
 */

/* --- 1. Integers --- */

/// u32 / I32
static u64 _hash_u32(const void *key)
{
	return hash_bytes(key, sizeof(u32));
}
static bool _eq_u32(const void *a, const void *b)
{
	return *(const u32 *)a == *(const u32 *)b;
}
const map_ops_t MAP_OPS_U32 = { .hash = _hash_u32, .equals = _eq_u32 };

/// u64 / I64
static u64 _hash_u64(const void *key)
{
	return hash_bytes(key, sizeof(u64));
}
static bool _eq_u64(const void *a, const void *b)
{
	return *(const u64 *)a == *(const u64 *)b;
}
const map_ops_t MAP_OPS_U64 = { .hash = _hash_u64, .equals = _eq_u64 };

/// uSIZE / Pointers (Address)
static u64 _hash_usize(const void *key)
{
	return hash_bytes(key, sizeof(usize));
}
static bool _eq_usize(const void *a, const void *b)
{
	return *(const usize *)a == *(const usize *)b;
}
const map_ops_t MAP_OPS_USIZE = { .hash = _hash_usize, .equals = _eq_usize };
const map_ops_t MAP_OPS_PTR = { .hash = _hash_usize,
				.equals = _eq_usize }; /// alias

/* --- 2. C-String --- */

static u64 _hash_cstr(const void *key)
{
	/// key stored in map is `char*`, passed by address -> `char**`
	const char *str = *(const char *const *)key;
	return hash_bytes(str, strlen(str));
}
static bool _eq_cstr(const void *a, const void *b)
{
	const char *s1 = *(const char *const *)a;
	const char *s2 = *(const char *const *)b;
	return strcmp(s1, s2) == 0;
}
const map_ops_t MAP_OPS_CSTR = { .hash = _hash_cstr, .equals = _eq_cstr };

/*
 * ==========================================================================
 * Internal Logic (Linear Probing)
 * ==========================================================================
 */

/// return true if found existing key, false if found empty slot for insert
static bool _find_slot(map_header_t *m, const void *key, usize *out_idx)
{
	if (m->cap == 0)
		return false;

	u64 hash = m->ops.hash(key);
	usize idx = (usize)(hash & (m->cap - 1)); /// cap is power of 2
	usize start_idx = idx;

	usize first_tomb = (usize)-1;

	do {
		u8 state = m->states[idx];

		if (state == _MAP_EMPTY) {
			/// not found. Use tombstone if available.
			if (first_tomb != (usize)-1)
				*out_idx = first_tomb;
			else
				*out_idx = idx;
			return false;
		}

		if (state == _MAP_TOMB) {
			if (first_tomb == (usize)-1)
				first_tomb = idx;
		} else if (state == _MAP_FULL) {
			/// compare keys
			void *slot_key = m->keys + (idx * m->key_size);
			if (m->ops.equals(key, slot_key)) {
				*out_idx = idx;
				return true; /// found
			}
		}

		idx = (idx + 1) & (m->cap - 1);
	} while (idx != start_idx);

	/// map full (should be prevented by load factor)
	if (first_tomb != (usize)-1) {
		*out_idx = first_tomb;
		return false;
	}

	massert(false, "Map is completely full (logic error)");
	return false;
}

static bool _map_resize(map_header_t *m, usize new_cap)
{
	/// use zalloc for states to init to _MAP_EMPTY (0)
	layout_t l_keys =
		layout(new_cap * m->key_size, 1); /// alignment simplified to 1
	layout_t l_vals = layout(new_cap * m->val_size, 1);
	layout_t l_states = layout(new_cap, 1);

	u8 *new_keys = (u8 *)allocer_alloc(m->alc, l_keys);
	u8 *new_vals = (u8 *)allocer_alloc(m->alc, l_vals);
	u8 *new_states = (u8 *)allocer_zalloc(m->alc, l_states);

	if (!new_keys || !new_vals || !new_states) {
		if (new_keys)
			allocer_free(m->alc, new_keys, l_keys);
		if (new_vals)
			allocer_free(m->alc, new_vals, l_vals);
		if (new_states)
			allocer_free(m->alc, new_states, l_states);
		return false;
	}

	/// create temp map to use _find_slot logic for rehash
	map_header_t new_m = *m;
	new_m.keys = new_keys;
	new_m.vals = new_vals;
	new_m.states = new_states;
	new_m.cap = new_cap;
	new_m.len = 0;
	new_m.occupied = 0;

	/// rehash all FULL entries
	for (usize i = 0; i < m->cap; ++i) {
		if (m->states[i] == _MAP_FULL) {
			void *k = m->keys + (i * m->key_size);
			void *v = m->vals + (i * m->val_size);

			usize idx;
			/// should always return false (not found) in a fresh map
			_find_slot(&new_m, k, &idx);

			memcpy(new_keys + (idx * m->key_size), k, m->key_size);
			memcpy(new_vals + (idx * m->val_size), v, m->val_size);
			new_states[idx] = _MAP_FULL;
			new_m.len++;
			new_m.occupied++;
		}
	}

	/// free old arrays
	if (m->cap > 0) {
		allocer_free(m->alc, m->keys, layout(m->cap * m->key_size, 1));
		allocer_free(m->alc, m->vals, layout(m->cap * m->val_size, 1));
		allocer_free(m->alc, m->states, layout(m->cap, 1));
	}

	*m = new_m;
	return true;
}

/*
 * ==========================================================================
 * Public Implementation
 * ==========================================================================
 */

bool _map_init_impl(anyptr map, allocer_t alc, map_ops_t ops, usize k_sz,
		    usize v_sz)
{
	map_header_t *m = (map_header_t *)map;
	m->keys = nullptr;
	m->vals = nullptr;
	m->states = nullptr;
	m->len = 0;
	m->cap = 0;
	m->occupied = 0;
	m->alc = alc;
	m->ops = ops;
	m->key_size = k_sz;
	m->val_size = v_sz;
	return true;
}

void _map_deinit_impl(anyptr map, usize k_sz, usize v_sz)
{
	map_header_t *m = (map_header_t *)map;
	if (m->cap > 0) {
		allocer_free(m->alc, m->keys, layout(m->cap * k_sz, 1));
		allocer_free(m->alc, m->vals, layout(m->cap * v_sz, 1));
		allocer_free(m->alc, m->states, layout(m->cap, 1));
	}
	m->cap = 0;
	m->len = 0;
}

bool _map_put_impl(anyptr map, const void *k_ptr, const void *v_ptr)
{
	map_header_t *m = (map_header_t *)map;

	/// load factor check (0.75)
	if (m->cap == 0 || (m->occupied + 1) * 4 >= m->cap * 3) {
		usize new_cap = (m->cap == 0) ? 8 : m->cap * 2;
		if (!_map_resize(m, new_cap))
			return false;
	}

	usize idx;
	bool exists = _find_slot(m, k_ptr, &idx);

	if (!exists) {
		/// new entry
		memcpy(m->keys + (idx * m->key_size), k_ptr, m->key_size);
		m->states[idx] = _MAP_FULL;
		m->len++;
		m->occupied++;
	}
	/// update value
	memcpy(m->vals + (idx * m->val_size), v_ptr, m->val_size);

	return true;
}

void *_map_get_impl(anyptr map, const void *k_ptr)
{
	map_header_t *m = (map_header_t *)map;
	if (m->len == 0)
		return nullptr;

	usize idx;
	if (_find_slot(m, k_ptr, &idx)) {
		return m->vals + (idx * m->val_size);
	}
	return nullptr;
}

bool _map_remove_impl(anyptr map, const void *k_ptr)
{
	map_header_t *m = (map_header_t *)map;
	if (m->len == 0)
		return false;

	usize idx;
	if (_find_slot(m, k_ptr, &idx)) {
		m->states[idx] = _MAP_TOMB;
		m->len--;
		/// occupied does NOT decrease
		return true;
	}
	return false;
}

void _map_clear_impl(anyptr map)
{
	map_header_t *m = (map_header_t *)map;
	if (m->cap > 0) {
		memset(m->states, _MAP_EMPTY, m->cap);
		m->len = 0;
		m->occupied = 0;
	}
}
