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

#include <core/macros.h>
#include <std/test.h>
#include <std/env.h>
#include <std/allocers/system.h>
#include <std/fs.h> /// for file_exists
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

/*
 * ==========================================================================
 * 1. Arguments Parsing
 * ==========================================================================
 */

TEST(args_complex_parsing)
{
	allocer_t sys = allocer_system();

	/// mock complex argv
	/// case: Program name, Flag, Value, Empty string argument, Another flag
	char *mock_argv[] = { "./compiler", "-o", "output.bin",
			      "", /// empty argument (valid in shell)
			      "--verbose" };
	int mock_argc = 5;

	args_t args;
	expect(args_init(&args, sys, mock_argc, mock_argv));

	/// 1. program Name
	expect(str_eq_cstr(args_program_name(&args), "./compiler"));

	/// 2. cursor check (initially 0)
	expect(args_remaining(&args) == usize_(5));

	/// 3. consumption loop
	/// consume prog name
	str_t s0 = args_next(&args);
	expect(str_eq_cstr(s0, "./compiler"));

	/// consume "-o"
	expect(args_has_next(&args));
	str_t s1 = args_next(&args);
	expect(str_eq_cstr(s1, "-o"));

	/// peek "output.bin"
	str_t p2 = args_peek(&args);
	expect(str_eq_cstr(p2, "output.bin"));
	/// cursor shouldn't move
	expect(str_eq(args_peek(&args), p2));

	/// consume "output.bin"
	str_t s2 = args_next(&args);
	expect(str_eq_cstr(s2, "output.bin"));

	/// consume Empty Arg
	str_t s3 = args_next(&args);
	expect(s3.len == 0);
	expect(s3.ptr !=
	       nullptr); /// should point to the empty string literal in argv

	/// consume "--verbose"
	str_t s4 = args_next(&args);
	expect(str_eq_cstr(s4, "--verbose"));

	/// 4. exhaustion
	expect(!args_has_next(&args));
	expect(args_remaining(&args) == 0);

	/// safety check: next on empty returns empty string (not crash)
	str_t s_end = args_next(&args);
	expect(str_is_empty(s_end));

	args_deinit(&args);
	return true;
}

TEST(args_iterator_macro)
{
	allocer_t sys = allocer_system();

	/// mock: ["./prog", "arg1", "arg2"]
	char *mock_argv[] = { "./prog", "arg1", "arg2" };
	int mock_argc = 3;

	args_t args;
	expect(args_init(&args, sys, mock_argc, mock_argv));

	/// 1. consume Program Name manually
	str_t prog = args_next(&args);
	expect(str_eq_cstr(prog, "./prog"));

	/// 2. iterate remaining
	int count = 0;
	args_foreach(arg, &args)
	{
		count++;
		if (count == 1)
			expect(str_eq_cstr(arg, "arg1"));
		if (count == 2)
			expect(str_eq_cstr(arg, "arg2"));
	}
	expect_eq(count, 2);

	/// 3. verify exhaustion
	expect(!args_has_next(&args));

	args_deinit(&args);
	return true;
}

TEST(args_iterator_empty)
{
	allocer_t sys = allocer_system();

	/// mock: ["./prog"] (No arguments)
	char *mock_argv[] = { "./prog" };
	int mock_argc = 1;

	args_t args;
	expect(args_init(&args, sys, mock_argc, mock_argv));

	/// skip prog
	args_next(&args);

	/// loop should not run
	int count = 0;
	args_foreach(arg, &args)
	{
		unused(arg);
		count++;
	}
	expect_eq(count, 0);

	args_deinit(&args);
	return true;
}

/*
 * ==========================================================================
 * 2. Environment Variables
 * ==========================================================================
 */

TEST(env_vars_lifecycle)
{
	allocer_t sys = allocer_system();
	string_t s;
	expect(string_init(&s, sys, 0));

	const char *KEY = "FLUF_TEST_VAR_12345";

	/// 1. ensure clean state
	env_unset(KEY);
	expect(env_get(KEY, &s) == false);

	/// 2. set & Get
	expect(env_set(KEY, "Value1"));
	expect(env_get(KEY, &s));
	expect(str_eq_cstr(string_as_str(&s), "Value1"));

	/// 3. overwrite
	string_clear(&s);
	expect(env_set(KEY, "Value2_Overwritten"));
	expect(env_get(KEY, &s));
	expect(str_eq_cstr(string_as_str(&s), "Value2_Overwritten"));

	/// 4. unset
	expect(env_unset(KEY));
	string_clear(&s);
	expect(env_get(KEY, &s) == false);

	string_deinit(&s);
	return true;
}

/*
 * ==========================================================================
 * 3. CWD (Current Working Directory)
 * ==========================================================================
 */

TEST(env_cwd_robustness)
{
	allocer_t sys = allocer_system();
	string_t s;
	expect(string_init(&s, sys, 0));

	/// 1. basic Get
	expect(env_current_dir(&s));
	expect(string_len(&s) > 0);

	/// verify it's a valid path that exists
	/// (Directories are files in fs model usually)
	const char *first_cwd_cstr = string_cstr(&s);
	unused(first_cwd_cstr);
	/// log_info("CWD is: %s", first_cwd_cstr); /// optional debug

	/// we can verify it exists using std/fs (if implemented to check dirs)
	/// or just assume if getcwd returns success, it exists.

	/// 2. test Append Semantics (Zero-Copy Append Logic)
	/// add a prefix to the string buffer to ensure env_current_dir
	/// appends to it instead of overwriting it.
	string_clear(&s);
	expect(string_append_cstr(&s, "PREFIX:"));
	usize prefix_len = string_len(&s);

	expect(env_current_dir(&s));

	/// length should be prefix + path
	expect(string_len(&s) > prefix_len);
	/// check prefix integrity
	expect(strncmp(string_cstr(&s), "PREFIX:", 7) == 0);

	/// 3. change Directory Test
	/// go up one level
	string_clear(&s);
	if (chdir("..") == 0) {
		expect(env_current_dir(&s));
		/// should be different (shorter usually, or different folder)
		/// but hard to assert exact string without assuming hierarchy.
		/// at least it should return success.
	}

	string_deinit(&s);
	return true;
}

int main()
{
	RUN(args_complex_parsing);
	RUN(env_vars_lifecycle);
	RUN(env_cwd_robustness);
	RUN(args_iterator_macro);
	RUN(args_iterator_empty);

	SUMMARY();
}
