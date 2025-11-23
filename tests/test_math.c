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
#include <core/math.h>
#include <core/type.h>

/*
 * ==========================================================================
 * Bitwise & Alignment Tests
 * ==========================================================================
 */

TEST(math_power_of_two)
{
	/// check basic powers of two
	expect(is_power_of_two(usize_(1)));
	expect(is_power_of_two(usize_(2)));
	expect(is_power_of_two(usize_(4096)));
	expect(is_power_of_two(usize_(1) << 63));

	/// check non powers of two
	expect(!is_power_of_two(usize_(3)));
	expect(!is_power_of_two(usize_(100)));
	expect(!is_power_of_two((usize_(1) << 63) + 1));

	/// check edge case: 0 is not a power of two
	expect(!is_power_of_two(usize_(0)));

	return true;
}

TEST(math_alignment_logic)
{
	/// 1. align_up
	/// check standard alignment scenarios
	expect_eq(align_up(usize_(5), usize_(4)), usize_(8));
	expect_eq(align_up(usize_(1), usize_(4)), usize_(4));

	/// check when already aligned (should stay same)
	expect_eq(align_up(usize_(8), usize_(4)), usize_(8));
	expect_eq(align_up(usize_(0), usize_(4)), usize_(0));

	/// check alignment to 1 (identity)
	expect_eq(align_up(usize_(123), usize_(1)), usize_(123));

	/// 2. align_down
	/// check truncating down logic
	expect_eq(align_down(usize_(7), usize_(4)), usize_(4));
	expect_eq(align_down(usize_(4), usize_(4)), usize_(4));
	expect_eq(align_down(usize_(3), usize_(4)), usize_(0));

	/// 3. is_aligned
	/// check alignment verification
	expect(is_aligned(usize_(1024), usize_(512)));
	expect(is_aligned(usize_(0), usize_(8)));
	expect(!is_aligned(usize_(1025), usize_(512)));

	return true;
}

TEST(math_alignment_death)
{
	/// check if align_up panics when align is not a power of two
	/// e.g. aligning to 3 is invalid
	expect_panic(align_up(usize_(10), usize_(3)));

	/// check align_down panic condition
	expect_panic(align_down(usize_(10), usize_(5)));

	/// check is_aligned panic condition
	expect_panic(is_aligned(usize_(10), usize_(6)));

	return true;
}

/*
 * ==========================================================================
 * Intrinsics Tests
 * ==========================================================================
 */

TEST(math_intrinsics)
{
	/// 1. Count Leading Zeros (clz64)
	/// check basic cases
	expect_eq(clz64(u64_(0xF000000000000000)), 0);
	expect_eq(clz64(u64_(1)), 63);

	/// check edge case: 0 input should return 64
	expect_eq(clz64(u64_(0)), 64);

	/// 2. Count Trailing Zeros (ctz64)
	/// check basic cases
	expect_eq(ctz64(u64_(8)), 3);
	expect_eq(ctz64(u64_(1)), 0);
	expect_eq(ctz64(u64_(0x8000000000000000)), 63);

	/// check edge case: 0 input should return 64
	expect_eq(ctz64(u64_(0)), 64);

	/// 3. Population Count (popcount64)
	/// check basic bit counting
	expect_eq(popcount64(u64_(0)), 0);
	expect_eq(popcount64(u64_(0xFFFFFFFFFFFFFFFF)), 64);
	expect_eq(popcount64(u64_(0b10101)), 3);

	return true;
}

/*
 * ==========================================================================
 * Algorithm Tests
 * ==========================================================================
 */

TEST(math_next_pow2)
{
	/// check small numbers
	expect_eq(next_power_of_two(usize_(0)), usize_(1));
	expect_eq(next_power_of_two(usize_(1)), usize_(1));
	expect_eq(next_power_of_two(usize_(2)), usize_(2));

	/// check rounding behavior
	expect_eq(next_power_of_two(usize_(3)), usize_(4));
	expect_eq(next_power_of_two(usize_(100)), usize_(128));

	/// check large boundary values
	usize large_pow2 = usize_(1) << 62;
	expect_eq(next_power_of_two(large_pow2), large_pow2);
	expect_eq(next_power_of_two(large_pow2 - 1), large_pow2);

	/// check overflow handling
	/// if the result would overflow usize, it should return 0
	usize huge = (usize)-1;
	expect_eq(next_power_of_two(huge), usize_(0));

	return true;
}

/*
 * ==========================================================================
 * Type-Safe Macros Tests
 * ==========================================================================
 */

TEST(math_min_max_clamp_types)
{
	/// check standard integer usage
	expect_eq(min(10, 20), 10);
	expect_eq(max(10, 20), 20);

	/// check floating point usage to verify type safety
	/// using strict equality for simple floats here is safe enough
	expect(min(1.5f, 2.5f) == 1.5f);
	expect(max(3.14, 10.0) == 10.0);

	/// check mixed signedness (careful with implicit conversion rules)
	/// this is just to ensure macros don't break syntax
	int neg = -5;
	int pos = 5;
	expect_eq(min(neg, pos), -5);
	expect_eq(max(neg, pos), 5);

	/// check clamp logic
	/// case 1: value < low
	expect_eq(clamp(5, 10, 20), 10);
	/// case 2: value > high
	expect_eq(clamp(25, 10, 20), 20);
	/// case 3: low <= value <= high
	expect_eq(clamp(15, 10, 20), 15);

	return true;
}

TEST(math_side_effects)
{
	/// verify that macros do not double evaluate arguments
	int x = 1;
	int y = 10;

	/// if implemented incorrectly as ((a)<(b)?(a):(b)), x would increment twice
	int res_min = min(x++, y);

	/// check result correctness
	expect_eq(res_min, 1);

	/// check x only incremented once
	expect_eq(x, 2);

	/// check clamp side effects
	int z = 15;
	/// clamp(z++, 10, 20) should return 15 and z becomes 16
	expect_eq(clamp(z++, 10, 20), 15);
	expect_eq(z, 16);

	return true;
}

int main()
{
	RUN(math_power_of_two);
	RUN(math_alignment_logic);
	RUN(math_alignment_death);
	RUN(math_intrinsics);
	RUN(math_next_pow2);
	RUN(math_min_max_clamp_types);
	RUN(math_side_effects);

	SUMMARY();
}
