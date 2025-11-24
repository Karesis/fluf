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
#include <std/fs/srcmanager.h>
#include <std/allocers/system.h>

TEST(srcman_basic_flow)
{
	allocer_t sys = allocer_system();
	srcmanager_t mgr;
	expect(srcmanager_init(&mgr, sys));

	/// file 1: 10 bytes + newline = 11? No.
	/// "Hello\nWorld" -> len 11
	/// h e l l o \n W o r l d
	/// 0 1 2 3 4 5  6 7 8 9 10
	/// line 1 starts at 0. Line 2 starts at 6.
	str_t c1 = str("Hello\nWorld");
	usize id1 = srcmanager_add(&mgr, str("main.c"), c1);
	expect_eq(id1, usize_(0));

	/// lookup 'H' (offset 0)
	srcloc_t loc;
	expect(srcmanager_lookup(&mgr, 0, &loc));
	expect(str_eq_cstr(str_from_cstr(loc.filename), "main.c"));
	expect_eq(loc.line, usize_(1));
	expect_eq(loc.col, usize_(1));

	/// lookup 'W' (offset 6)
	expect(srcmanager_lookup(&mgr, 6, &loc));
	expect_eq(loc.line, usize_(2));
	expect_eq(loc.col, usize_(1));

	/// lookup 'd' (offset 10)
	expect(srcmanager_lookup(&mgr, 10, &loc));
	expect_eq(loc.line, usize_(2));
	expect_eq(loc.col, usize_(5));

	srcmanager_deinit(&mgr);
	return true;
}

TEST(srcman_multiple_files)
{
	allocer_t sys = allocer_system();
	srcmanager_t mgr;
	expect(srcmanager_init(&mgr, sys));

	/// file 1 (len 5): "12345" -> Offsets 0-4
	srcmanager_add(&mgr, str("A"), str("12345"));

	/// file 2 (len 3): "abc"   -> Offsets 5-7
	srcmanager_add(&mgr, str("B"), str("abc"));

	srcloc_t loc;

	/// check File A limit
	expect(srcmanager_lookup(&mgr, 4, &loc));
	expect(str_eq_cstr(str_from_cstr(loc.filename), "A"));
	expect_eq(loc.col, usize_(5));

	/// check File B start
	expect(srcmanager_lookup(&mgr, 5, &loc));
	expect(str_eq_cstr(str_from_cstr(loc.filename), "B"));
	expect_eq(loc.col, usize_(1)); /// relative to B start

	/// check File B middle
	expect(srcmanager_lookup(&mgr, 6, &loc)); /// 'b'
	expect_eq(loc.col, usize_(2));

	/// out of bounds
	expect(srcmanager_lookup(&mgr, 8, &loc) == false);

	srcmanager_deinit(&mgr);
	return true;
}

TEST(srcman_line_content)
{
	allocer_t sys = allocer_system();
	srcmanager_t mgr;
	expect(srcmanager_init(&mgr, sys));

	/// "Line1\nLine2\r\nLine3"
	/// l1: 0-4, \n at 5
	/// l2: 6-10, \r at 11, \n at 12
	/// l3: 13-17
	srcmanager_add(&mgr, str("test"), str("Line1\nLine2\r\nLine3"));

	/// fetch content of Line 1 (via offset 0)
	str_t l1 = srcmanager_get_line_content(&mgr, 0);
	expect(str_eq_cstr(l1, "Line1"));

	/// fetch content of Line 2 (via offset 6)
	str_t l2 = srcmanager_get_line_content(&mgr, 6);
	expect(str_eq_cstr(l2, "Line2")); /// should verify \r is stripped

	/// fetch content of Line 3 (via offset 13)
	str_t l3 = srcmanager_get_line_content(&mgr, 13);
	expect(str_eq_cstr(l3, "Line3"));

	srcmanager_deinit(&mgr);
	return true;
}

int main()
{
	RUN(srcman_basic_flow);
	RUN(srcman_multiple_files);
	RUN(srcman_line_content);
	SUMMARY();
}
