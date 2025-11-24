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
#include <std/map.h>
#include <std/allocers/system.h>
#include <core/hash.h>

/*
 * ==========================================================================
 * Helper: Custom Struct Key
 * ==========================================================================
 */

typedef struct {
	int x;
	int y;
} Point;

static u64 _hash_point(const void *key)
{
	return hash_bytes(key, sizeof(Point));
}

static bool _eq_point(const void *a, const void *b)
{
	const Point *pa = (const Point *)a;
	const Point *pb = (const Point *)b;
	return pa->x == pb->x && pa->y == pb->y;
}

static const map_ops_t MAP_OPS_POINT = { .hash = _hash_point,
					 .equals = _eq_point };

/*
 * ==========================================================================
 * Tests
 * ==========================================================================
 */

TEST(map_basic_u32)
{
	allocer_t sys = allocer_system();

	/// define map(u32, int)
	map(u32, int) m;
	expect(map_init(m, sys, MAP_OPS_U32));

	/// 1. insert new
	expect(map_put(m, 10, 100));
	expect(map_put(m, 20, 200));
	expect_eq(map_len(m), usize_(2));

	/// 2. get existing
	int *val1 = map_get(m, 10);
	expect(val1 != nullptr);
	expect_eq(*val1, 100);

	int *val2 = map_get(m, 20);
	expect(val2 != nullptr);
	expect_eq(*val2, 200);

	/// 3. get non-existing
	int *val3 = map_get(m, 30);
	expect(val3 == nullptr);

	/// 4. overwrite
	expect(map_put(m, 10, 101));
	expect_eq(map_len(m), usize_(2)); /// length should not change
	expect_eq(*map_get(m, 10), 101);

	map_deinit(m);
	return true;
}

TEST(map_string_keys)
{
	allocer_t sys = allocer_system();

	/// define map(const char*, float)
	map(const char *, float) m;
	expect(map_init(m, sys, MAP_OPS_CSTR));

	/// note: keys are not copied by the map (it stores pointers)
	/// user must ensure string lifetime. literals are safe.
	expect(map_put(m, "apple", 1.5f));
	expect(map_put(m, "banana", 2.5f));

	/// check existence
	expect(map_get(m, "apple") != nullptr);
	expect_eq(*map_get(m, "banana"), 2.5f);
	expect(map_get(m, "cherry") == nullptr);

	/// check string equality logic
	/// pass a different pointer but same content (if possible)
	char buffer[] = { 'a', 'p', 'p', 'l', 'e', '\0' };
	expect(map_get(m, buffer) != nullptr);

	map_deinit(m);
	return true;
}

TEST(map_custom_struct_key)
{
	allocer_t sys = allocer_system();

	/// define map(Point, int)
	map(Point, int) m;
	expect(map_init(m, sys, MAP_OPS_POINT));

	Point p1 = { 1, 2 };
	Point p2 = { 3, 4 };
	Point p3 = { 1, 2 }; /// same value as p1

	expect(map_put(m, p1, 100));
	expect(map_put(m, p2, 200));

	/// check lookup by value (p3 has same content as p1)
	int *v = map_get(m, p3);
	expect(v != nullptr);
	expect_eq(*v, 100);

	map_deinit(m);
	return true;
}

TEST(map_growth_and_rehash)
{
	allocer_t sys = allocer_system();
	map(u64, u64) m;
	expect(map_init(m, sys, MAP_OPS_U64));

	/// default capacity is usually 8 or 16
	/// let's insert enough items to trigger resize
	/// assumed max load factor is 0.75

	usize count = 100;
	for (usize i = 0; i < count; ++i) {
		expect(map_put(m, i, i * 10));
	}

	expect_eq(map_len(m), count);
	/// capacity should have grown (must be power of 2)
	expect(map_cap(m) >= 128);

	/// verify all data is still there after rehash
	for (usize i = 0; i < count; ++i) {
		u64 *val = map_get(m, i);
		expect(val != nullptr);
		expect_eq(*val, i * 10);
	}

	map_deinit(m);
	return true;
}

TEST(map_tombstone_logic)
{
	allocer_t sys = allocer_system();
	map(u32, int) m;
	expect(map_init(m, sys, MAP_OPS_U32));

	/// we need to trigger a collision chain
	/// since we use fnv-1a, hard to predict collisions perfectly
	/// but filling up to near capacity usually creates chains

	/// 1. insert A, B, C
	expect(map_put(m, 1, 10));
	expect(map_put(m, 2, 20));
	expect(map_put(m, 3, 30));

	/// 2. remove B (middle of potential chain, or just an element)
	expect(map_remove(m, 2));
	expect_eq(map_len(m), usize_(2));
	expect(map_get(m, 2) == nullptr);

	/// 3. get A and C (should still be accessible)
	expect_eq(*map_get(m, 1), 10);
	expect_eq(*map_get(m, 3), 30);

	/// 4. insert D (re-using tombstone potentially)
	expect(map_put(m, 4, 40));
	expect_eq(map_len(m), usize_(3));

	/// 5. re-insert B
	expect(map_put(m, 2, 22));
	expect_eq(*map_get(m, 2), 22);

	map_deinit(m);
	return true;
}

TEST(map_clear_reuse)
{
	allocer_t sys = allocer_system();
	map(u32, int) m;
	expect(map_init(m, sys, MAP_OPS_U32));

	expect(map_put(m, 1, 1));
	expect(map_put(m, 2, 2));

	usize old_cap = map_cap(m);

	/// clear
	map_clear(m);
	expect_eq(map_len(m), usize_(0));
	expect_eq(map_cap(m), old_cap); /// capacity preserved

	/// reuse
	expect(map_put(m, 3, 3));
	expect_eq(map_len(m), usize_(1));
	expect_eq(*map_get(m, 3), 3);

	/// old keys should be gone
	expect(map_get(m, 1) == nullptr);

	map_deinit(m);
	return true;
}

int main()
{
	RUN(map_basic_u32);
	RUN(map_string_keys);
	RUN(map_custom_struct_key);
	RUN(map_growth_and_rehash);
	RUN(map_tombstone_logic);
	RUN(map_clear_reuse);

	SUMMARY();
}
