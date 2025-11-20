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
	auto l1 = layout(usize(10), usize(1));
	expect_eq(l1.size, usize(10));
	expect_eq(l1.align, usize(1));

	/// check 8 bytes alignment
	auto l2 = layout(usize(128), usize(8));
	expect_eq(l2.size, usize(128));
	expect_eq(l2.align, usize(8));

	return true;
}

TEST(layout_creation_death)
{
	/// check panic on invalid alignment (not power of two)
	/// alignment 3 is invalid
	expect_panic(layout(usize(10), usize(3)));

	/// alignment 0 is invalid (0 is not power of two)
	expect_panic(layout(usize(10), usize(0)));

	return true;
}

TEST(layout_of_primitives)
{
	/// check basic integer types
	auto int_l = layout_of(i32);
	expect_eq(int_l.size, usize(4));
	expect_eq(int_l.align, usize(alignof(i32)));

	auto char_l = layout_of(char);
	expect_eq(char_l.size, usize(1));
	expect_eq(char_l.align, usize(1));

	return true;
}

TEST(layout_of_structs)
{
	/// check packed structure (size 1, align 1)
	auto l1 = layout_of(struct Packed);
	expect_eq(l1.size, usize(1));
	expect_eq(l1.align, usize(1));

	/// check padded structure
	/// this verifies that sizeof() includes padding correctly
	auto l2 = layout_of(struct PadMe);

	/// PadMe: u8(1) + padding(3) + u32(4) = 8 bytes
	expect_eq(l2.size, usize(8));
	expect_eq(l2.align, usize(4));

	/// double check with compiler intrinsics
	expect_eq(l2.size, usize(sizeof(struct PadMe)));
	expect_eq(l2.align, usize(alignof(struct PadMe)));

	return true;
}

TEST(layout_of_arrays)
{
	/// check standard array layout
	/// layout_of_array(T, N) -> size = sizeof(T) * N, align = alignof(T)

	/// 5 integers
	auto arr = layout_of_array(i32, 5);
	expect_eq(arr.size, usize(20)); /// 4 * 5
	expect_eq(arr.align, usize(alignof(i32)));

	/// check zero length array (edge case)
	auto empty = layout_of_array(i32, 0);
	expect_eq(empty.size, usize(0));
	expect_eq(empty.align, usize(alignof(i32)));

	/// check array of padded structs
	/// 2 PadMe structs: 8 * 2 = 16 bytes
	auto struct_arr = layout_of_array(struct PadMe, 2);
	expect_eq(struct_arr.size, usize(16));
	expect_eq(struct_arr.align, usize(alignof(struct PadMe)));

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
