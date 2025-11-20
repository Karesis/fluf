#include <std/test.h>
#include <core/type.h>
#include <core/span.h>
#include <core/msg.h>

/*
 * ==========================================================================
 * Main Logics
 * ==========================================================================
 */

TEST(span_construction_rules)
{
	/// 1. [Positive]
	auto s1 = span(10, 20);
	expect_eq(s1.start, usize(10));
	expect_eq(s1.end, usize(20));
	expect_eq(span_len(s1), usize(10));

	/// 2. [Edge] (Sanitization)
	/// .end = (start > end ? start : end)
	/// so span(20, 10) should be [20, 20)
	auto s2 = span(20, 10);
	expect_eq(s2.start, usize(20));
	expect_eq(s2.end, usize(20));
	expect_eq(span_len(s2), usize(0));

	/// 3. [Edge] (Empty Span)
	auto s3 = span(5, 5);
	expect_eq(span_len(s3), usize(0));

	auto s4 = span_from_len(100, 50);
	expect_eq(s4.start, usize(100));
	expect_eq(s4.end, usize(150));

	return true;
}

TEST(span_comparison)
{
	auto a = span(1, 5);
	auto b = span(1, 5);
	auto c = span(1, 6);

	expect(span_cmp(a, b) == true);
	expect(span_cmp(a, c) == false);

	return true;
}

TEST(span_merge_logic)
{
	/// case 1: overlapping
	/// [0..10] + [5..15] -> [0..15]
	auto s1 = span(0, 10);
	auto s2 = span(5, 15);
	auto res1 = span_merge(s1, s2);
	expect(span_cmp(res1, span(0, 15)));

	/// case 2: contained
	/// [0..20] + [5..10] -> [0..20]
	auto s3 = span(0, 20);
	auto s4 = span(5, 10);
	auto res2 = span_merge(s3, s4);
	expect(span_cmp(res2, span(0, 20)));

	/// case 3: disjoint
	/// [0..5] + [10..15] -> [0..15]
	auto s5 = span(0, 5);
	auto s6 = span(10, 15);
	auto res3 = span_merge(s5, s6);
	expect(span_cmp(res3, span(0, 15)));

	return true;
}

/*
 * ==========================================================================
 * Macros And Iters
 * ==========================================================================
 */

TEST(macro_loop_execution)
{
	/// 1. check `for_span`
	auto s = span(0, 5);
	usize sum = 0;
	usize count = 0;
	for_span(i, s)
	{
		sum += i;
		count++;
	}

	expect_eq(count, usize(5));
	expect_eq(sum, usize(0 + 1 + 2 + 3 + 4)); /// 10

	/// 2. check empty iter
	auto empty = span(10, 10);
	for_span(i, empty)
	{
		unused(i);
		unreach();
	}

	return true;
}

TEST(macro_foreach_generic)
{
	/// 1. loop int
	int sum_i = 0;
	foreach(i, 0, 5)
	{
		sum_i += i;
	}
	expect_eq(sum_i, 10);

	/// 2. loop pointer
	char buf[] = "hello";
	char *buf_addr = &buf[0];
	char *ptr_sum = buf;
	/// from `buf_addr` to `buf_addr+5`
	foreach(p, buf_addr, buf_addr + 5)
	{
		expect(is_type(p, char *)); /// check auto type
		unused(p);
		ptr_sum++;
	}
	expect(ptr_sum == buf + 5);

	return true;
}

int main()
{
	RUN(span_construction_rules);
	RUN(span_comparison);
	RUN(span_merge_logic);
	RUN(macro_loop_execution);
	RUN(macro_foreach_generic);

	SUMMARY();
}
