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

#include <std/test.h>
#include <core/mem/allocer.h>
#include <core/type.h>

/*
 * ==========================================================================
 * 1. Mock Allocator: Static Bump Allocator
 * ==========================================================================
 * A simple allocator that hands out memory from a static buffer.
 * It does NOT support freeing individual items.
 */

#define BUMP_SIZE 1024

struct BumpState {
	u8 buffer[BUMP_SIZE];
	usize offset;
	usize alloc_count; // Statistics
	usize zalloc_count; // Statistics
};

// Implementation of .alloc
static anyptr bump_alloc(anyptr self, layout_t layout)
{
	auto s = (struct BumpState *)self;
	s->alloc_count++;

	// 1. Align the current offset
	auto aligned_offset = align_up(s->offset, layout.align);

	// 2. Check OOM
	if (aligned_offset + layout.size > BUMP_SIZE) {
		return nullptr;
	}

	// 3. Bump ptr
	anyptr ptr = &s->buffer[aligned_offset];
	s->offset = aligned_offset + layout.size;

	return ptr;
}

// Implementation of .free (No-op)
static void bump_free(anyptr self, anyptr ptr, layout_t layout)
{
	unused(self);
	unused(ptr);
	unused(layout);
	// Bump allocator cannot free individual blocks.
}

// Implementation of .zalloc (Optional override)
// We override it to count calls, but logic is same as fallback
static anyptr bump_zalloc(anyptr self, layout_t layout)
{
	auto s = (struct BumpState *)self;
	s->zalloc_count++;

	// Reuse alloc logic
	anyptr ptr = bump_alloc(self, layout);
	if (ptr) {
		memset(ptr, 0, layout.size);
	}
	return ptr;
}

// The VTable
static const allocer_vtable_t bump_vtable = {
	.alloc = bump_alloc,
	.free = bump_free,
	.realloc = nullptr, // Use fallback
	.zalloc = bump_zalloc // Override
};

/*
 * ==========================================================================
 * Tests
 * ==========================================================================
 */

TEST(allocer_vtable_dispatch)
{
	/// 1. Setup the Allocator
	struct BumpState state = { .offset = 0,
				   .alloc_count = 0,
				   .zalloc_count = 0 };
	allocer_t a = { .self = &state, .vtable = &bump_vtable };

	/// 2. Test alloc
	/// Expect offset to move
	auto p1 = alloc_type(a, int);
	expect(p1 != nullptr);
	*p1 = 42;
	expect_eq(state.alloc_count, usize(1));

	/// Check alignment impact
	/// int is usually 4-byte aligned. Current offset should be around 4.
	expect(state.offset >= usize(4));

	/// 3. Test zalloc (Override)
	/// Expect zalloc_count to increase
	auto p2 = zalloc_type(a, int);
	expect(p2 != nullptr);
	expect_eq(*p2, 0); // Should be zeroed
	expect_eq(state.zalloc_count, usize(1));

	/// 4. Test free (No-op)
	/// Should not crash
	free_type(a, p1);

	return true;
}

TEST(allocer_fallback_realloc)
{
	/// 1. Setup
	struct BumpState state = { .offset = 0 };
	allocer_t a = { .self = &state, .vtable = &bump_vtable };

	/// 2. Alloc small array
	/// layout: i32 * 2 (8 bytes)
	auto arr = alloc_array(a, i32, 2);
	arr[0] = 10;
	arr[1] = 20;

	auto old_layout = layout_of_array(i32, 2);
	auto new_layout = layout_of_array(i32, 4);

	/// 3. Realloc to larger size
	/// Since realloc is nullptr in vtable, this triggers:
	/// alloc(new) -> memcpy -> free(old)
	auto new_arr = (i32 *)allocer_realloc(a, arr, old_layout, new_layout);

	expect(new_arr != nullptr);
	expect(new_arr !=
	       arr); // Must be a new pointer (bump allocator never reuses in place)

	/// Check data preservation
	expect_eq(new_arr[0], 10);
	expect_eq(new_arr[1], 20);

	return true;
}

TEST(allocer_oom_handling)
{
	struct BumpState state = { .offset = 0 };
	allocer_t a = { .self = &state, .vtable = &bump_vtable };

	/// Try to allocate more than BUMP_SIZE
	auto huge = layout(usize(BUMP_SIZE + 1), usize(1));
	anyptr p = allocer_alloc(a, huge);

	/// Should return nullptr gracefully
	expect(p == nullptr);

	return true;
}

int main()
{
	RUN(allocer_vtable_dispatch);
	RUN(allocer_fallback_realloc);
	RUN(allocer_oom_handling);
	SUMMARY();
}
