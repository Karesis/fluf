#include <std/test.h>
#include <std/math/bitset.h>
#include <std/allocers/system.h>

/*
 * ==========================================================================
 * 1. Basic Lifecycle & Access
 * ==========================================================================
 */

TEST(bitset_lifecycle)
{
	allocer_t sys = allocer_system();
	bitset_t bs;

	/// 1. init (Zeroed by default)
	expect(bitset_init(&bs, sys, 128));
	expect_eq(bitset_count(&bs), usize_(0));
	expect(bitset_none(&bs));

	/// 2. basic Set/Test
	bitset_set(&bs, 0);
	expect(bitset_test(&bs, 0));
	expect(bitset_test(&bs, 1) == false);

	bitset_set(&bs, 127); /// last bit
	expect(bitset_test(&bs, 127));

	/// 3. clear
	bitset_clear(&bs, 0);
	expect(bitset_test(&bs, 0) == false);

	/// 4. flip
	bitset_flip(&bs, 10);
	expect(bitset_test(&bs, 10));
	bitset_flip(&bs, 10);
	expect(bitset_test(&bs, 10) == false);

	/// 5. assign
	bitset_assign(&bs, 50, true);
	expect(bitset_test(&bs, 50));
	bitset_assign(&bs, 50, false);
	expect(!bitset_test(&bs, 50));

	bitset_deinit(&bs);
	return true;
}

/*
 * ==========================================================================
 * 2. Boundary & Word Alignment (The Tricky Part)
 * ==========================================================================
 */

TEST(bitset_word_boundaries)
{
	allocer_t sys = allocer_system();
	bitset_t bs;

	/// 64 bits = exactly 1 word
	/// 65 bits = 2 words (1 bit in second word)
	expect(bitset_init(&bs, sys, 65));

	/// test Bit 63 (End of word 0)
	bitset_set(&bs, 63);
	expect(bitset_test(&bs, 63));

	/// test Bit 64 (Start of word 1)
	bitset_set(&bs, 64);
	expect(bitset_test(&bs, 64));

	expect_eq(bitset_count(&bs), usize_(2));

	bitset_deinit(&bs);
	return true;
}

TEST(bitset_masking_integrity)
{
	/// this is the most important test for the implementation!
	/// it verifies that operations do not corrupt the unused bits
	/// in the last word.

	allocer_t sys = allocer_system();
	bitset_t bs;

	/// size 10. allocated 1 word (64 bits).
	/// bits 10-63 are "unused" and MUST remain 0.
	expect(bitset_init(&bs, sys, 10));

	/// 1. set All
	/// if logic is wrong, it might set all 64 bits to 1.
	/// correct logic should only set bits 0-9.
	bitset_set_all(&bs);

	expect_eq(bitset_count(&bs), usize_(10)); /// nOT 64!
	expect(bitset_all(&bs)); /// should be true for the valid range

	/// 2. flip All
	/// 11...1 -> 00...0
	bitset_flip_all(&bs);
	expect_eq(bitset_count(&bs), usize_(0));

	/// flip again -> 11...1 (10 bits)
	bitset_flip_all(&bs);
	expect_eq(bitset_count(&bs), usize_(10));

	bitset_deinit(&bs);
	return true;
}

/*
 * ==========================================================================
 * 3. Set Operations (Algebra)
 * ==========================================================================
 */

TEST(bitset_algebra)
{
	allocer_t sys = allocer_system();
	bitset_t a, b;
	expect(bitset_init(&a, sys, 64));
	expect(bitset_init(&b, sys, 64));

	/// a = {0, 1}
	bitset_set(&a, 0);
	bitset_set(&a, 1);

	/// b = {1, 2}
	bitset_set(&b, 1);
	bitset_set(&b, 2);

	/// union (A | B) -> {0, 1, 2}
	/// use a clone to check result without destroying A
	bitset_t *u = bitset_clone(&a);
	bitset_union(u, &b);
	expect(bitset_test(u, 0));
	expect(bitset_test(u, 1));
	expect(bitset_test(u, 2));
	expect_eq(bitset_count(u), usize_(3));
	bitset_drop(u);

	/// intersection (A & B) -> {1}
	bitset_t *i = bitset_clone(&a);
	bitset_intersect(i, &b);
	expect(!bitset_test(i, 0));
	expect(bitset_test(i, 1));
	expect(!bitset_test(i, 2));
	expect_eq(bitset_count(i), usize_(1));
	bitset_drop(i);

	/// difference (A - B) -> {0}
	bitset_t *d = bitset_clone(&a);
	bitset_difference(d, &b);
	expect(bitset_test(d, 0));
	expect(!bitset_test(d, 1));
	bitset_drop(d);

	/// symmetric Difference (XOR) -> {0, 2}
	bitset_t *x = bitset_clone(&a);
	bitset_xor(x, &b);
	expect(bitset_test(x, 0));
	expect(!bitset_test(x, 1)); /// 1^1 = 0
	expect(bitset_test(x, 2));
	bitset_drop(x);

	bitset_deinit(&a);
	bitset_deinit(&b);
	return true;
}

/*
 * ==========================================================================
 * 4. Safety & Death Tests
 * ==========================================================================
 */

TEST(bitset_oob_safety)
{
	allocer_t sys = allocer_system();
	bitset_t bs;
	expect(bitset_init(&bs, sys, 10));

	/// valid access
	bitset_set(&bs, 9);

	/// invalid access (Index == Size)
	expect_panic(bitset_set(&bs, 10));

	/// invalid access (Way out)
	expect_panic(bitset_test(&bs, 100));

	bitset_deinit(&bs);
	return true;
}

TEST(bitset_mismatch_safety)
{
	allocer_t sys = allocer_system();
	bitset_t a, b;

	expect(bitset_init(&a, sys, 10));
	expect(bitset_init(&b, sys, 20)); /// different size

	/// set operations require equal sizes
	expect_panic(bitset_union(&a, &b));

	bitset_deinit(&a);
	bitset_deinit(&b);
	return true;
}

/*
 * ==========================================================================
 * 5. Iters
 * ==========================================================================
 */

TEST(bitset_iterator)
{
	allocer_t sys = allocer_system();
	bitset_t bs;
	expect(bitset_init(&bs, sys, 200)); /// spans 4 words (64*3 + 8)

	/// set explicit bits
	bitset_set(&bs, 1);
	bitset_set(&bs, 63); /// end of word 0
	bitset_set(&bs, 64); /// start of word 1
	bitset_set(&bs, 100); /// middle of word 1
	bitset_set(&bs, 150); /// word 2
	/// word 3 is empty

	usize indices[10];
	usize count = 0;

	usize idx;
	bitset_foreach(idx, &bs)
	{
		if (count < 10)
			indices[count++] = idx;
	}

	expect_eq(count, usize_(5));
	expect_eq(indices[0], usize_(1));
	expect_eq(indices[1], usize_(63));
	expect_eq(indices[2], usize_(64));
	expect_eq(indices[3], usize_(100));
	expect_eq(indices[4], usize_(150));

	bitset_deinit(&bs);
	return true;
}

int main()
{
	RUN(bitset_lifecycle);
	RUN(bitset_word_boundaries);
	RUN(bitset_masking_integrity);
	RUN(bitset_algebra);
	RUN(bitset_oob_safety);
	RUN(bitset_mismatch_safety);
	RUN(bitset_iterator);

	SUMMARY();
}
