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
#include <std/allocers/bump.h>
#include <core/mem/allocer.h>
#include <core/math.h>
#include <string.h>
#include <stdlib.h> /// for malloc/free in mock

/*
 * ==========================================================================
 * 1. Mock / Tracking Allocator
 * ==========================================================================
 * This allocator wraps system malloc but tracks statistics.
 * It also allows simulating allocation failures.
 */

struct MockState {
	usize alloc_calls;
	usize free_calls;
	usize bytes_allocated; /// current active bytes
	usize max_bytes; /// peak usage
	bool simulate_oom; /// if true, alloc returns nullptr
};

static anyptr mock_alloc(anyptr self, layout_t layout)
{
	struct MockState *s = (struct MockState *)self;
	if (s->simulate_oom)
		return nullptr;

	s->alloc_calls++;
	s->bytes_allocated += layout.size;
	if (s->bytes_allocated > s->max_bytes)
		s->max_bytes = s->bytes_allocated;

	/// use system malloc for actual memory
	/// ensure alignment is respected (using aligned_alloc or simple malloc for test)
	/// for simplicity in test, we assume malloc is sufficiently aligned for chunks (usually 16 bytes)
	return malloc(layout.size);
}

static void mock_free(anyptr self, anyptr ptr, layout_t layout)
{
	struct MockState *s = (struct MockState *)self;
	s->free_calls++;
	s->bytes_allocated -= layout.size;
	free(ptr);
}

static const allocer_vtable_t MOCK_VTABLE = { .alloc = mock_alloc,
					      .free = mock_free,
					      .realloc = nullptr,
					      .zalloc = nullptr };

static allocer_t mock_allocator(struct MockState *state)
{
	*state = (struct MockState){ 0 }; /// reset state
	return (allocer_t){ .self = state, .vtable = &MOCK_VTABLE };
}

/*
 * ==========================================================================
 * 2. Basic Logic & Lifecycle
 * ==========================================================================
 */

TEST(bump_lifecycle_stack)
{
	struct MockState mock_st;
	allocer_t backing = mock_allocator(&mock_st);

	/// 1. init on stack
	bump_t bump;
	bump_init(&bump, backing, 1);

	/// at init, it does NOT allocate any chunk yet (lazy init)
	/// implementation sets `get_empty_chunk()`, so 0 allocations expected.
	expect_eq(mock_st.alloc_calls, usize_(0));

	/// 2. first allocation triggers chunk creation
	int *i = bump_alloc_type(&bump, int);
	expect(i != nullptr);
	*i = 123;

	/// now we expect 1 chunk allocation from backing
	expect_eq(mock_st.alloc_calls, usize_(1));

	/// 3. deinit
	bump_deinit(&bump);

	/// Should free the chunk
	expect_eq(mock_st.free_calls, usize_(1));
	expect_eq(mock_st.bytes_allocated, usize_(0)); /// no leaks

	return true;
}

TEST(bump_lifecycle_heap)
{
	struct MockState mock_st;
	allocer_t backing = mock_allocator(&mock_st);

	/// 1. new on heap (allocates bump_t struct itself)
	bump_t *b = bump_new(backing, 1);
	expect(b != nullptr);

	/// backing should have alloc'd sizeof(bump_t)
	expect_eq(mock_st.alloc_calls, usize_(1));

	/// 2. drop
	bump_drop(b);

	/// should free bump_t struct
	expect_eq(mock_st.free_calls, usize_(1));
	expect_eq(mock_st.bytes_allocated, usize_(0));

	return true;
}

/*
 * ==========================================================================
 * 3. Allocation Logic (Direction & Alignment)
 * ==========================================================================
 */

TEST(bump_direction_and_layout)
{
	struct MockState mock_st;
	allocer_t backing = mock_allocator(&mock_st);
	bump_t bump;
	bump_init(&bump, backing, 1);

	/// alloc 1: a single byte
	u8 *p1 = bump_alloc_type(&bump, u8);
	*p1 = 0xAA;

	/// alloc 2: another byte
	u8 *p2 = bump_alloc_type(&bump, u8);
	*p2 = 0xBB;

	/// check downward growth logic
	/// since bump grows down, p2 should be at a lower address than p1
	expect((uptr)p2 < (uptr)p1);

	/// Check adjacency (no alignment padding for u8)
	expect_eq((uptr)p1 - (uptr)p2, usize_(1));

	bump_deinit(&bump);
	return true;
}

TEST(bump_alignment_strict)
{
	struct MockState mock_st;
	allocer_t backing = mock_allocator(&mock_st);

	/// init with min_align = 1
	bump_t bump;
	bump_init(&bump, backing, 1);

	/// 1. alloc u8 (align 1)
	bump_alloc_type(&bump, u8);

	/// 2. alloc u64 (align 8)
	/// The allocator must align the pointer DOWN to a multiple of 8
	u64 *p64 = bump_alloc_type(&bump, u64);
	expect(is_aligned((uptr)p64, 8));

	/// 3. alloc with manual high alignment (e.g., 128 bytes)
	void *p_high = bump_alloc(&bump, 16, 128);
	expect(is_aligned((uptr)p_high, 128));

	bump_deinit(&bump);
	return true;
}

/*
 * ==========================================================================
 * 4. Chunk Growth & Reset
 * ==========================================================================
 */

TEST(bump_growth_and_reset)
{
	struct MockState mock_st;
	allocer_t backing = mock_allocator(&mock_st);
	bump_t bump;
	bump_init(&bump, backing, 1);

	/// 1. force multiple chunks
	/// default chunk is ~4KB. Let's alloc 3KB twice.
	/// this will force two separate chunks because 3+3 > 4.
	void *p1 = bump_alloc(&bump, 3000, 1);
	void *p2 = bump_alloc(&bump, 3000, 1); /// should trigger new chunk

	expect(p1 != nullptr);
	expect(p2 != nullptr);

	/// expect 2 chunks allocated
	expect_eq(mock_st.alloc_calls, usize_(2));

	/// 2. test reset
	/// bump_reset keeps the CURRENT chunk, but frees OLD chunks.
	bump_reset(&bump);

	/// we had 2 chunks. Reset keeps 1 (current), frees 1 (old).
	expect_eq(mock_st.free_calls, usize_(1));

	/// 3. alloc again
	/// should fit in the kept chunk
	void *p3 = bump_alloc(&bump, 100, 1);
	expect(p3 != nullptr);

	/// no new allocs needed from backing
	expect_eq(mock_st.alloc_calls, usize_(2)); // still 2 total calls

	bump_deinit(&bump);
	/// final cleanup frees the last chunk
	expect_eq(mock_st.free_calls, usize_(2));
	expect_eq(mock_st.bytes_allocated, usize_(0));

	return true;
}

/*
 * ==========================================================================
 * 5. Limits & Failures
 * ==========================================================================
 */

TEST(bump_limits)
{
	struct MockState mock_st;
	allocer_t backing = mock_allocator(&mock_st);
	bump_t bump;
	bump_init(&bump, backing, 1);

	/// set a small limit (e.g., 5000 bytes)
	/// note: The limit checks against `allocated_bytes` which includes overhead/padding.
	/// first chunk is likely 4096 bytes.
	bump_set_allocation_limit(&bump, 5000);

	/// Alloc 1: 3000 bytes (OK, fits in 1st chunk of 4096)
	expect(bump_alloc(&bump, 3000, 1) != nullptr);

	/// alloc 2: 3000 bytes
	/// logic: need new chunk.
	/// current total = 4096. new chunk would be ~4096.
	/// Total = 8192 > 5000 Limit.
	/// should fail.
	expect(bump_alloc(&bump, 3000, 1) == nullptr);

	bump_deinit(&bump);
	return true;
}

TEST(bump_oom_backing)
{
	struct MockState mock_st;
	allocer_t backing = mock_allocator(&mock_st);
	bump_t bump;
	bump_init(&bump, backing, 1);

	/// simulate OOM in backing allocator
	mock_st.simulate_oom = true;

	/// try alloc
	void *p = bump_alloc(&bump, 100, 1);
	expect(p == nullptr);

	bump_deinit(&bump);
	return true;
}

/*
 * ==========================================================================
 * 6. Adapters & Helpers
 * ==========================================================================
 */

TEST(bump_as_allocer_vtable)
{
	struct MockState mock_st;
	allocer_t backing = mock_allocator(&mock_st);
	bump_t bump;
	bump_init(&bump, backing, 1);

	/// convert to generic interface
	allocer_t generic = bump_allocer(&bump);

	/// use generic API
	layout_t l = layout_of(int);
	int *ptr = (int *)allocer_alloc(generic, l);
	expect(ptr != nullptr);
	*ptr = 99;

	/// zalloc via generic
	int *zptr = (int *)allocer_zalloc(generic, l);
	expect(zptr != nullptr);
	expect(*zptr == 0);

	bump_deinit(&bump);
	return true;
}

TEST(bump_string_helper)
{
	struct MockState mock_st;
	allocer_t backing = mock_allocator(&mock_st);
	bump_t bump;
	bump_init(&bump, backing, 1);

	const char *src = "hello bump";
	char *dst = bump_alloc_cstr(&bump, src);

	expect(dst != nullptr);
	expect(strcmp(dst, src) == 0);
	expect(dst != src); // must be a copy

	bump_deinit(&bump);
	return true;
}

int main()
{
	RUN(bump_lifecycle_stack);
	RUN(bump_lifecycle_heap);
	RUN(bump_direction_and_layout);
	RUN(bump_alignment_strict);
	RUN(bump_growth_and_reset);
	RUN(bump_limits);
	RUN(bump_oom_backing);
	RUN(bump_as_allocer_vtable);
	RUN(bump_string_helper);

	SUMMARY();
}
