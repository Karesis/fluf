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
#include <std/fs/dir.h>
#include <std/fs.h> /// for file_write, file_remove
#include <std/allocers/system.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define mkdir(p, m) _mkdir(p)
#define rmdir _rmdir
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

/// helper to setup sandbox
/// sandbox/
///   a.txt
///   sub/
///     b.txt
static bool setup_sandbox()
{
	/// best effort mkdir
#if defined(_WIN32)
	mkdir("sandbox", 0);
	mkdir("sandbox/sub", 0);
#else
	mkdir("sandbox", 0755);
	mkdir("sandbox/sub", 0755);
#endif
	expect(file_write("sandbox/a.txt", str("a")));
	expect(file_write("sandbox/sub/b.txt", str("b")));
	return true;
}

static void teardown_sandbox()
{
	file_remove("sandbox/sub/b.txt");
	rmdir("sandbox/sub");
	file_remove("sandbox/a.txt");
	rmdir("sandbox");
}

/// callback to count files
typedef struct {
	int files;
	int dirs;
} WalkStats;

static bool count_cb(const char *path, dir_entry_type_t type, void *userdata)
{
	WalkStats *st = (WalkStats *)userdata;
	if (type == DIR_ENTRY_DIR)
		st->dirs++;
	if (type == DIR_ENTRY_FILE)
		st->files++;

	unused(path);
	/// optional: verify path contains "sandbox"
	/// dbg("Walk: %s", path);
	return true; /// continue
}

TEST(dir_walk_basic)
{
	expect(setup_sandbox());

	allocer_t sys = allocer_system();
	WalkStats st = { 0 };

	expect(dir_walk(sys, "sandbox", count_cb, &st));

	/// expect:
	/// dirs: "sandbox/sub" (1)
	/// files: "sandbox/a.txt", "sandbox/sub/b.txt" (2)
	expect_eq(st.dirs, 1);
	expect_eq(st.files, 2);

	teardown_sandbox();
	return true;
}

int main()
{
	RUN(dir_walk_basic);
	SUMMARY();
}
