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
#include <core/msg.h>
#include <core/type.h>
#include <core/result.h>
/// for strcmp
#include <string.h>
/// for bool
#include <stdbool.h>

/*
 * ==========================================================================
 * Struct Utils
 * ==========================================================================
 */

/// Option<int>
defOption(int, OptInt);

/// Result<int, const char*>
defResult(int, const char *, ResIntStr);

/// Result<int, int> (crucial for testing generic fmt() support)
defResult(int, int, ResInt);

/*
 * ==========================================================================
 * Function Utils (For Testing `try` (?) Flow)
 * ==========================================================================
 */

/// scenario A: try succeeds, execution should continue.
static ResIntStr helper_try_success(void)
{
	ResIntStr intermediate = ok(10);
	/// if this succeeds, v becomes 10; otherwise, it returns err immediately.
	int v = try(intermediate);
	return (ResIntStr)ok(v + 5); /// should return ok(15)
}

/// scenario B: try fails, should return immediately.
static ResIntStr helper_try_failure(void)
{
	ResIntStr intermediate = err("failure trigger");
	/// this will return err("failure trigger") immediately.
	int v = try(intermediate);

	/// the following code should never execute.
	unused(v);
	unreach();
	return (ResIntStr)ok(999);
}

/// scenario C: try option fails.
static OptInt helper_try_option_fail(void)
{
	OptInt none_val = none;
	/// when option fails, it returns None (zero-initialized struct) immediately.
	int v = try(none_val);

	unused(v);
	unreach();
	return (OptInt)some(999);
}

/*
 * ==========================================================================
 * Main Tests
 * ==========================================================================
 */

TEST(reflection_magic)
{
	/// [Core Logic] verify internal layout assumptions.
	OptInt o = some(1);
	ResIntStr r = ok(1);

	/// check tag size constants.
	expect_eq(sizeof(o._tag), usize(1));
	expect_eq(sizeof(r._tag), usize(2));

	/// verify `_is_result` macro accuracy.
	expect(_is_result(o) == false);
	expect(_is_result(r) == true);

	return true;
}

TEST(option_basic_ops)
{
	/// 1. test some variant
	OptInt s = some(42);
	expect(is_some(s));
	expect(!is_none(s));
	expect(is_ok(s)); /// `Duck Typing`: Option also has `is_ok` field.
	expect_eq(unwrap(s), 42);

	/// 2. test none variant
	OptInt n = none;
	expect(is_none(n));
	expect(!is_some(n));

	/// check `unwrap_or`
	expect_eq(unwrap_or(n, 100), 100);

	/// check `unwrap(None)` (death test)
	/// expected panic: "unwrap failed: Option is None"
	expect_panic(unwrap(n));

	return true;
}

TEST(result_basic_ops)
{
	/// 1. test `O`k variant
	ResIntStr r_ok = ok(200);
	expect(is_ok(r_ok));
	expect(!is_err(r_ok));
	expect_eq(unwrap(r_ok), 200);

	/// 2. test `Err` variant
	ResIntStr r_err = err("oops");
	expect(is_err(r_err));
	expect(!is_ok(r_err));

	/// check `unwrap_or`
	expect_eq(unwrap_or(r_err, 300), 300);

	/// check error message content
	expect(strcmp(r_err.err, "oops") == 0);

	return true;
}

TEST(result_smart_features)
{
	/// test the new generic `unwrap` and `expect` capabilities.
	/// this proves that your library can handle non-string errors gracefully!

	/// 1. test `expect` with STRING error (classic behavior)
	/// Should panic with: "Database error: Connection timeout"
	ResIntStr r_str = err("Connection timeout");
	expect_panic(check(r_str, "Database error"));

	/// 2. test `expect` with INT error (new generic behavior)
	/// should panic with: "HTTP Request failed: 404"
	/// This verifies that fmt() correctly picked "%d" for the int error.
	ResInt r_int = err(404);
	expect_panic(check(r_int, "HTTP Request failed"));

	/// 3. test smart `unwrap` with INT error
	/// should panic with: "unwrap failed: 500"
	/// this verifies unwrap() is not dumb anymore.
	ResInt r_int2 = err(500);
	expect_panic(unwrap(r_int2));

	return true;
}

TEST(if_let_control_flow)
{
	OptInt opt = some(123);
	bool entered = false;

	/// test binding success
	if_let(val, opt)
	{
		expect_eq(val, 123);
		entered = true;
	}
	expect(entered);

	/// test None path (should not enter)
	OptInt none_v = none;
	if_let(val2, none_v)
	{
		unused(val2);
		unreach(); /// should not be reached
	}

	return true;
}

TEST(try_operator_semantics)
{
	/// 1. test success propagation
	ResIntStr s = helper_try_success();
	expect(is_ok(s));
	expect_eq(unwrap(s), 15); /// 10 + 5

	/// 2. test failure propagation (Result)
	ResIntStr f = helper_try_failure();
	expect(is_err(f));
	/// check if the error string was preserved
	expect(strcmp(f.err, "failure trigger") == 0);

	/// 3. test failure propagation (Option)
	OptInt opt_f = helper_try_option_fail();
	expect(is_none(opt_f));

	return true;
}

TEST(option_mutable_semantics)
{
	/// Options have "Pass by Value" semantics,
	/// behaving exactly like a normal C struct.
	OptInt a = some(10);
	OptInt b = a; /// Full copy occurs here

	b.val = 20;
	expect_eq(a.val, 10); /// Original remains unchanged
	expect_eq(b.val, 20);

	return true;
}

TEST(result_mutable_semantics)
{
	/// 1. basic value type test (int).
	/// verify that `Result` follows "Pass by Value" (Struct Copy) semantics.
	ResInt a = ok(10);
	ResInt b = a; /// full memory copy (memcpy)

	b.val = 20; /// modify the copy
	expect_eq(a.val, 10); /// the original remains unchanged
	expect_eq(b.val, 20);

	/// 2. pointer type test (Shallow Copy)
	/// important: struct assignment only copies the pointer address
	/// not the underlying data it points to
	char buffer[] = "Original";
	ResIntStr r1 = err(buffer); /// r1 holds address of `buffer`
	ResIntStr r2 = r1; /// r2 holds the same address

	/// modify the struct member of r2 to point to a new string
	/// this changes r2's pointer, but not r1's pointer
	r2.err = "Changed";

	/// r1 still points to the original buffer
	expect(strcmp(r1.err, "Original") == 0);
	expect(strcmp(r2.err, "Changed") == 0);

	return true;
}

int main()
{
	RUN(reflection_magic);
	RUN(option_basic_ops);
	RUN(result_basic_ops);
	RUN(result_smart_features);
	RUN(if_let_control_flow);
	RUN(try_operator_semantics);
	RUN(option_mutable_semantics);
	RUN(result_mutable_semantics);

	SUMMARY();
}
