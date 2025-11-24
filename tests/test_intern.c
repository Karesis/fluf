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
#include <std/strings/intern.h>
#include <std/allocers/system.h>
#include <stdio.h> /// for snprintf

/*
 * ==========================================================================
 * Basic Logic & Lifecycle
 * ==========================================================================
 */

TEST(intern_lifecycle)
{
	allocer_t sys = allocer_system();

	/// 1. stack initialization
	interner_t it;
	expect(intern_init(&it, sys)); /// satisfy nodiscard

	expect_eq(intern_count(&it), usize_(0));

	intern_deinit(&it);

	/// 2. heap allocation
	interner_t *heap_it = intern_new(sys);
	expect(heap_it !=
	       nullptr); /// satisfy nodiscard (implicitly checked by if, but explicit check is better)

	intern_drop(heap_it);

	return true;
}

TEST(intern_deduplication)
{
	allocer_t sys = allocer_system();
	interner_t it;
	expect(intern_init(&it, sys));

	/// 1. intern first time
	symbol_t sym1 = intern_cstr(&it, "hello");
	expect_eq(sym1.id, u32_(0));

	/// 2. intern same content, different pointer
	char buf[] = "hello";
	symbol_t sym2 = intern_cstr(&it, buf);

	/// must be same id
	expect_eq(sym2.id, sym1.id);
	expect(sym_eq(sym1, sym2));

	/// 3. intern different string
	symbol_t sym3 = intern_cstr(&it, "world");
	expect(sym3.id != sym1.id);
	expect_eq(sym3.id, u32_(1));

	/// 4. resolve check
	str_t s1 = intern_resolve(&it, sym1);
	str_t s3 = intern_resolve(&it, sym3);

	expect(str_eq_cstr(s1, "hello"));
	expect(str_eq_cstr(s3, "world"));

	intern_deinit(&it);
	return true;
}

/*
 * ==========================================================================
 * Advanced & Edge Cases
 * ==========================================================================
 */

TEST(intern_empty_string)
{
	allocer_t sys = allocer_system();
	interner_t it;
	expect(intern_init(&it, sys));

	/// intern empty string
	symbol_t sym = intern_cstr(&it, "");

	/// resolve back
	str_t s = intern_resolve(&it, sym);
	expect_eq(s.len, usize_(0));
	expect(str_is_empty(s));

	const char *cstr = intern_resolve_cstr(&it, sym);
	expect_eq(strcmp(cstr, ""), 0);

	intern_deinit(&it);
	return true;
}

TEST(intern_binary_safety)
{
	allocer_t sys = allocer_system();
	interner_t it;
	expect(intern_init(&it, sys));

	/// construct a string with embedded null: "a\0b"
	/// length is 3
	char raw[] = { 'a', '\0', 'b' };
	str_t input = str_from_parts(raw, 3);

	/// intern it
	/// this relies on bump_dup_str working correctly with slices
	symbol_t sym = intern(&it, input);

	/// resolve
	str_t out = intern_resolve(&it, sym);

	/// check length and content
	expect_eq(out.len, usize_(3));
	expect(out.ptr[0] == 'a');
	expect(out.ptr[1] == '\0');
	expect(out.ptr[2] == 'b');

	/// check cstr resolution (will stop at first null)
	const char *cstr = intern_resolve_cstr(&it, sym);
	expect_eq(strlen(cstr), usize_(1));
	expect(cstr[0] == 'a');

	intern_deinit(&it);
	return true;
}

/*
 * ==========================================================================
 * Stress Test
 * ==========================================================================
 */

TEST(intern_massive_usage)
{
	allocer_t sys = allocer_system();
	interner_t it;
	expect(intern_init(&it, sys));

	const int N = 1000;
	char buf[64];

	/// 1. insert N unique strings
	for (int i = 0; i < N; ++i) {
		snprintf(buf, sizeof(buf), "key_%d", i);
		symbol_t sym = intern_cstr(&it, buf);

		/// ids should be sequential: 0, 1, 2 ...
		expect_eq(sym.id, u32_(i));
	}

	expect_eq(intern_count(&it), usize_(N));

	/// 2. verify all strings are retrievable
	for (int i = 0; i < N; ++i) {
		snprintf(buf, sizeof(buf), "key_%d", i);

		/// using intern again should return existing id
		symbol_t sym = intern_cstr(&it, buf);
		expect_eq(sym.id, u32_(i));

		/// resolve should match content
		str_t s = intern_resolve(&it, sym);
		expect(str_eq_cstr(s, buf));
	}

	intern_deinit(&it);
	return true;
}

int main()
{
	RUN(intern_lifecycle);
	RUN(intern_deduplication);
	RUN(intern_empty_string);
	RUN(intern_binary_safety);
	RUN(intern_massive_usage);

	SUMMARY();
}
