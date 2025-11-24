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
#include <std/vec.h>
#include <std/allocers/system.h>

typedef struct {
	int x, y;
} Point;

TEST(vec_basic_int)
{
	allocer_t sys = allocer_system();

	/// 1. definition
	vec(int) v;
	expect(vec_init(v, sys, 0)); /// lazy init

	/// 2. push (trigger growth)
	for (int i = 0; i < 10; ++i) {
		expect(vec_push(v, i));
	}
	expect_eq(vec_len(v), usize_(10));
	expect(vec_cap(v) >= 10);

	/// 3. access
	for (int i = 0; i < 10; ++i) {
		expect_eq(v.data[i], i);
		expect_eq(vec_at(v, i), i);
	}

	/// 4. pop
	expect_eq(vec_pop(v), 9);
	expect_eq(vec_len(v), usize_(9));

	vec_deinit(v);
	return true;
}

TEST(vec_struct_type)
{
	allocer_t sys = allocer_system();
	vec(Point) points;
	expect(vec_init(points, sys, 4));

	Point p1 = { 10, 20 };
	vec_push(points, p1);

	/// literal push
	vec_push(points, ((Point){ .x = 30, .y = 40 }));

	expect_eq(vec_len(points), usize_(2));
	expect_eq(points.data[0].x, 10);
	expect_eq(points.data[1].y, 40);

	/// iterator
	int sum_x = 0;
	vec_foreach(it, points)
	{
		sum_x += it->x;
	}
	expect_eq(sum_x, 40);

	vec_deinit(points);
	return true;
}

TEST(vec_alignment_check)
{
	/// create a type with weird alignment
	typedef struct {
		u8 a;
		/// padding
		alignas(64) u8 b;
	} AlignedItem;

	allocer_t sys = allocer_system();
	vec(AlignedItem) v;
	expect(vec_init(v, sys, 1));

	vec_push(v, ((AlignedItem){ .a = 0 }));

	/// verify data pointer is aligned to 64
	expect(is_aligned((uptr)v.data, 64));

	vec_deinit(v);
	return true;
}

TEST(vec_reserve_logic)
{
	allocer_t sys = allocer_system();
	vec(int) v;
	expect(vec_init(v, sys, 0));

	/// reserve space
	expect(vec_reserve(v, 100));
	expect(vec_cap(v) >= 100);
	expect_eq(vec_len(v), usize_(0));

	/// pointer stability check (should be same if no reallocation needed)
	int *ptr1 = v.data;
	expect(vec_reserve(v, 50)); /// request less than current cap
	expect(v.data == ptr1);

	vec_deinit(v);
	return true;
}

TEST(vec_heap_lifecycle)
{
	/// use system allocator for strict leak checking
	allocer_t sys = allocer_system();

	/// 1. create IntVec on heap
	/// syntax: vec(int) *ptr = ...
	auto v = vec_new(sys, int, 10);

	expect(v != nullptr);
	expect(vec_cap(*v) >= 10);
	expect(vec_len(*v) == 0);

	/// 2. operations (dereference needed)
	vec_push(*v, 100);
	vec_push(*v, 200);

	expect_eq(vec_len(*v), usize_(2));
	expect_eq(vec_at(*v, 0), 100);

	/// 3. drop
	/// should free both the data array and the struct v
	vec_drop(v);

	return true;
}

TEST(vec_heap_struct_type)
{
	allocer_t sys = allocer_system();

	/// test with a struct type
	typedef struct {
		int id;
	} Item;

	auto v = vec_new(sys, Item, 2);
	expect(v != nullptr);

	vec_push(*v, ((Item){ .id = 1 }));
	expect_eq(vec_at(*v, 0).id, 1);

	vec_drop(v);

	return true;
}

int main()
{
	RUN(vec_basic_int);
	RUN(vec_struct_type);
	RUN(vec_alignment_check);
	RUN(vec_reserve_logic);
	RUN(vec_heap_lifecycle);
	RUN(vec_heap_struct_type);
	SUMMARY();
}
