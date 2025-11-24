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

#include <std/vec.h>
#include <core/math.h>

/*
 * Internal layout matching the macro definition.
 */
typedef struct {
	u8 *data;
	usize len;
	usize cap;
	allocer_t alc;
} vec_header_t;

bool _vec_init_impl(anyptr vec_struct, allocer_t alc, usize cap,
		    usize item_size, usize align)
{
	vec_header_t *v = (vec_header_t *)vec_struct;
	v->len = 0;
	v->cap = 0;
	v->alc = alc;
	v->data = nullptr;

	if (cap > 0) {
		/// overflow check
		usize total_bytes;
		if (checked_mul(cap, item_size, &total_bytes))
			return false;

		layout_t l = layout(total_bytes, align);
		v->data = (u8 *)allocer_alloc(alc, l);
		if (!v->data)
			return false;
		v->cap = cap;
	}
	return true;
}

void _vec_deinit_impl(anyptr vec_struct, usize item_size, usize align)
{
	vec_header_t *v = (vec_header_t *)vec_struct;
	if (v->data) {
		/// we must assume current capacity is valid
		usize total_bytes = v->cap * item_size;
		layout_t l = layout(total_bytes, align);
		allocer_free(v->alc, v->data, l);
	}
	v->data = nullptr;
	v->len = 0;
	v->cap = 0;
}

static bool _vec_realloc_internal(vec_header_t *v, usize new_cap,
				  usize item_size, usize align)
{
	usize old_bytes, new_bytes;

	/// check overflows
	if (checked_mul(v->cap, item_size, &old_bytes))
		return false;
	if (checked_mul(new_cap, item_size, &new_bytes))
		return false;

	layout_t old_l = layout(old_bytes, align);
	layout_t new_l = layout(new_bytes, align);

	u8 *new_data = (u8 *)allocer_realloc(v->alc, v->data, old_l, new_l);
	if (!new_data)
		return false;

	v->data = new_data;
	v->cap = new_cap;
	return true;
}

bool _vec_grow_impl(anyptr vec_struct, usize item_size, usize align)
{
	vec_header_t *v = (vec_header_t *)vec_struct;

	/// strategy: Double capacity, start at 8
	usize new_cap = (v->cap == 0) ? 8 : v->cap * 2;

	/// check overflow of capacity itself (usize limit)
	if (new_cap < v->cap)
		return false;

	return _vec_realloc_internal(v, new_cap, item_size, align);
}

bool _vec_reserve_impl(anyptr vec_struct, usize additional, usize item_size,
		       usize align)
{
	vec_header_t *v = (vec_header_t *)vec_struct;

	usize needed;
	if (checked_add(v->len, additional, &needed))
		return false;

	if (needed <= v->cap)
		return true; /// already enough space

	/// growth strategy: power of two (POT).
	/// even for explicit reservations, we align capacity to the next power of two.
	/// this prevents immediate reallocation if the user pushes slightly beyond
	/// the reserved count, maintaining amortized O(1) insertion performance.
	usize new_cap = next_power_of_two(needed);
	if (new_cap < needed)
		new_cap = needed; /// handle next_pow2 overflow/0 case

	return _vec_realloc_internal(v, new_cap, item_size, align);
}
