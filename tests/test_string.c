#include <std/test.h>
#include <std/strings/string.h>
#include <std/allocers/system.h>

/*
 * ==========================================================================
 * 1. Basic Logic
 * ==========================================================================
 */

TEST(string_lifecycle)
{
	allocer_t sys = allocer_system();
	string_t s;

	/// init with lazy allocation (cap_hint = 0)
	expect(string_init(&s, sys, 0));
	expect(string_len(&s) == 0);
	expect(string_is_empty(&s));

	/// even when empty, cstr should be valid ""
	expect_eq(strcmp(string_cstr(&s), ""), 0);

	/// drop empty string
	string_deinit(&s);

	return true;
}

TEST(string_push_append)
{
	allocer_t sys = allocer_system();
	string_t s;
	expect(string_init(&s, sys, 4)); // Small hint

	/// push chars
	expect(string_push(&s, 'A'));
	expect(string_push(&s, 'B'));
	expect_eq(strcmp(string_cstr(&s), "AB"), 0);

	/// append C-String
	expect(string_append_cstr(&s, "CD"));
	expect_eq(strcmp(string_cstr(&s), "ABCD"), 0);

	/// append Slice
	str_t slice = str("EFG");
	expect(string_append(&s, slice));
	expect_eq(strcmp(string_cstr(&s), "ABCDEFG"), 0);

	/// check length
	expect_eq(string_len(&s), usize_(7));

	string_deinit(&s);
	return true;
}

/*
 * ==========================================================================
 * 2. Capacity & Reallocation
 * ==========================================================================
 */

TEST(string_growth_strategy)
{
	allocer_t sys = allocer_system();
	string_t s;
	expect(string_init(&s, sys, 2));

	/// note: Implementation enforces a min capacity.
	/// so initial_cap might be 16 even if we asked for 2.
	usize initial_cap = s.cap;

	/// instead of assuming 1 append triggers growth,
	/// we loop until we exceed the initial capacity.
	/// this makes the test independent of the specific min_cap value.
	while (s.len + 1 < initial_cap) {
		expect(string_push(&s, 'X'));
	}

	/// now push one more to break the camel's back
	expect(string_push(&s, '!'));

	/// verify growth happened
	expect(s.cap > initial_cap);

	string_deinit(&s);
	return true;
}

TEST(string_reserve_logic)
{
	allocer_t sys = allocer_system();
	string_t s;
	expect(string_init(&s, sys, 0));

	/// reserve space for 100 chars
	expect(string_reserve(&s, 100));
	expect(s.cap >= 101); // 100 + null terminator

	/// current length should still be 0
	expect_eq(string_len(&s), usize_(0));
	expect_eq(strcmp(string_cstr(&s), ""), 0);

	/// fill it up partially
	expect(string_append_cstr(&s, "Hello"));
	expect_eq(string_len(&s), usize_(5));

	string_deinit(&s);
	return true;
}

/*
 * ==========================================================================
 * 3. Formatting & Complex Ops
 * ==========================================================================
 */

TEST(string_formatting_complex)
{
	allocer_t sys = allocer_system();
	string_t *s = string_new(sys, 0);

	/// simple format
	expect(string_fmt(s, "Val: %d", 42));
	expect(str_eq(string_as_str(s), str("Val: 42")));

	/// append format
	expect(string_fmt(s, " - Hex: 0x%x", 0xFF));
	expect(str_eq(string_as_str(s), str("Val: 42 - Hex: 0xff")));

	/// large format (trigger realloc inside vsnprintf logic)
	/// create a string > default stack buffer if any
	char long_str[1024];
	memset(long_str, 'A', 1023);
	long_str[1023] = '\0';

	string_clear(s);
	expect(string_fmt(s, "%s", long_str));
	expect_eq(string_len(s), usize_(1023));
	expect(s->cap >= 1024);

	/// verify tail
	expect(s->data[1022] == 'A');
	expect(s->data[1023] == '\0');

	string_drop(s);
	return true;
}

TEST(string_clear_reuse)
{
	allocer_t sys = allocer_system();
	string_t s;
	expect(string_init(&s, sys, 10));

	expect(string_append_cstr(&s, "Hello World"));
	expect_eq(string_len(&s), usize_(11));

	/// clear
	string_clear(&s);
	expect_eq(string_len(&s), usize_(0));
	expect_eq(strcmp(string_cstr(&s), ""), 0);

	/// reuse (Capacity should be preserved)
	expect(s.cap >= 11); // Should NOT shrink

	expect(string_append_cstr(&s, "Reuse"));
	expect_eq(strcmp(string_cstr(&s), "Reuse"), 0);

	string_deinit(&s);
	return true;
}

TEST(string_view_interaction)
{
	allocer_t sys = allocer_system();
	string_t s;
	expect(string_init(&s, sys, 0));

	expect(string_append_cstr(&s, "foo bar"));

	/// convert to view
	str_t view = string_as_str(&s);
	expect_eq(view.len, usize_(7));
	expect(str_eq(view, str("foo bar")));

	/// use view APIs
	expect(str_starts_with(view, str("foo")));

	string_deinit(&s);
	return true;
}

int main()
{
	RUN(string_lifecycle);
	RUN(string_push_append);
	RUN(string_growth_strategy);
	RUN(string_reserve_logic);
	RUN(string_formatting_complex);
	RUN(string_clear_reuse);
	RUN(string_view_interaction);

	SUMMARY();
}
