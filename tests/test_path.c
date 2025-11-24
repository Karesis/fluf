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
#include <std/fs/path.h>
#include <std/allocers/system.h>

TEST(path_query_components)
{
	/// 1. extension
	expect(str_eq_cstr(path_ext(str("test.c")), "c"));
	expect(str_eq_cstr(path_ext(str("archive.tar.gz")), "gz"));
	expect(str_is_empty(path_ext(str("Makefile"))));
	expect(str_is_empty(
		path_ext(str("dir.d/file")))); /// dot in dir, not file

	/// 2. file Name
	expect(str_eq_cstr(path_file_name(str("/usr/bin/ls")), "ls"));
	expect(str_eq_cstr(path_file_name(str("src/main.c")), "main.c"));
	expect(str_eq_cstr(path_file_name(str("file")), "file"));

	/// 3. dir Name
	expect(str_eq_cstr(path_dir_name(str("/usr/bin/ls")), "/usr/bin"));
	expect(str_eq_cstr(path_dir_name(str("src/main.c")), "src"));
	expect(str_is_empty(path_dir_name(str("file"))));
	expect(str_eq_cstr(path_dir_name(str("/file")), "/")); /// root check

	return true;
}

TEST(path_builder_logic)
{
	allocer_t sys = allocer_system();
	string_t s;
	expect(string_init(&s, sys, 0));

	/// 1. basic Push
	expect(path_push(&s, str("usr")));
	expect(str_eq_cstr(string_as_str(&s), "usr"));

	/// 2. push with sep insertion
	expect(path_push(&s, str("bin")));
	/// should imply "usr/bin" (or "\" on win)
#if defined(_WIN32)
	expect(str_eq_cstr(string_as_str(&s), "usr\\bin"));
#else
	expect(str_eq_cstr(string_as_str(&s), "usr/bin"));
#endif

	/// 3. push where component has sep (simple concat)
	expect(path_push(&s, str("local/lib"))); /// assumes Unix style input
	/// "usr/bin/local/lib"

	/// 4. set Ext
	expect(path_set_ext(&s, str("so")));
	/// "usr/bin/local/lib.so"
	expect(str_eq(path_ext(string_as_str(&s)), str("so")));

	/// 5. change Ext
	expect(path_set_ext(&s, str("dll")));
	expect(str_eq(path_ext(string_as_str(&s)), str("dll")));

	string_deinit(&s);
	return true;
}

int main()
{
	RUN(path_query_components);
	RUN(path_builder_logic);
	SUMMARY();
}
