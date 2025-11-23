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
#include <core/type.h>
#include <core/mem/layout.h>

/*
 * ==========================================================================
 * Helper Structures
 * ==========================================================================
 */

/// 1. structure with no padding
struct Packed {
	u8 a; /// 1 byte
};

/// 2. structure with padding
/// typical layout: [u8] [pad][pad][pad] [u32]
struct PadMe {
	u8 a;
	u32 b;
};

/// 3. structure with natural high alignment
struct BigAlign {
	long double x; /// usually 16 bytes aligned on x64
};

/*
 * ==========================================================================
 * Test Cases
 * ==========================================================================
 */

TEST(layout_manual_creation)
{
	/// check 1 byte alignment
	auto l1 = layout(usize_(10), usize_(1));
	expect_eq(l1.size, usize_(10));
	expect_eq(l1.align, usize_(1));

	/// check 8 bytes alignment
	auto l2 = layout(usize_(128), usize_(8));
	expect_eq(l2.size, usize_(128));
	expect_eq(l2.align, usize_(8));

	return true;
}

TEST(layout_creation_death)
{
	/// check panic on invalid alignment (not power of two)
	/// alignment 3 is invalid
	expect_panic(layout(usize_(10), usize_(3)));

	/// alignment 0 is invalid (0 is not power of two)
	expect_panic(layout(usize_(10), usize_(0)));

	return true;
}

TEST(layout_of_primitives)
{
	/// check basic integer types
	auto int_l = layout_of(i32);
	expect_eq(int_l.size, usize_(4));
	expect_eq(int_l.align, usize_(alignof(i32)));

	auto char_l = layout_of(char);
	expect_eq(char_l.size, usize_(1));
	expect_eq(char_l.align, usize_(1));

	return true;
}

TEST(layout_of_structs)
{
	/// check packed structure (size 1, align 1)
	auto l1 = layout_of(struct Packed);
	expect_eq(l1.size, usize_(1));
	expect_eq(l1.align, usize_(1));

	/// check padded structure
	/// this verifies that sizeof() includes padding correctly
	auto l2 = layout_of(struct PadMe);

	/// PadMe: u8(1) + padding(3) + u32(4) = 8 bytes
	expect_eq(l2.size, usize_(8));
	expect_eq(l2.align, usize_(4));

	/// double check with compiler intrinsics
	expect_eq(l2.size, usize_(sizeof(struct PadMe)));
	expect_eq(l2.align, usize_(alignof(struct PadMe)));

	return true;
}

TEST(layout_of_arrays)
{
	/// check standard array layout
	/// layout_of_array(T, N) -> size = sizeof(T) * N, align = alignof(T)

	/// 5 integers
	auto arr = layout_of_array(i32, 5);
	expect_eq(arr.size, usize_(20)); /// 4 * 5
	expect_eq(arr.align, usize_(alignof(i32)));

	/// check zero length array (edge case)
	auto empty = layout_of_array(i32, 0);
	expect_eq(empty.size, usize_(0));
	expect_eq(empty.align, usize_(alignof(i32)));

	/// check array of padded structs
	/// 2 PadMe structs: 8 * 2 = 16 bytes
	auto struct_arr = layout_of_array(struct PadMe, 2);
	expect_eq(struct_arr.size, usize_(16));
	expect_eq(struct_arr.align, usize_(alignof(struct PadMe)));

	return true;
}

int main()
{
	RUN(layout_manual_creation);
	RUN(layout_creation_death);
	RUN(layout_of_primitives);
	RUN(layout_of_structs);
	RUN(layout_of_arrays);

	SUMMARY();
}
