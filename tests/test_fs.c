#include <stdio.h>
#include <string.h>

// --- POSIX (用于 mkdir/rmdir) ---
#include <sys/stat.h> // mkdir
#include <unistd.h>   // rmdir

// --- Fluf 依赖 ---
#include <core/mem/allocer.h>
#include <core/msg/asrt.h>
#include <std/allocer/bump/bump.h>
#include <std/allocer/bump/glue.h>
#include <std/io/file.h> // 用 write_file_bytes 创建文件
#include <std/vec.h>     // 我们用 Vec<const char*> 来收集结果

// --- 我们要测试的模块 ---
#include <std/fs/dir.h>
#include <std/fs/path.h>

/**
 * @brief 测试 1：(单元测试) `path_get_extension`
 */
static void test_path_extension(void) {
  printf("--- Test: test_path_extension ---\n");

  asrt_msg(slice_equals_cstr(path_get_extension(SLICE_LITERAL("main.nyan")),
                             ".nyan"),
           "Failed .nyan");
  asrt_msg(slice_equals_cstr(path_get_extension(SLICE_LITERAL("main.")), "."),
           "Failed .");
  asrt_msg(slice_equals_cstr(path_get_extension(SLICE_LITERAL("main")), ""),
           "Failed no-ext");
  asrt_msg(
      slice_equals_cstr(path_get_extension(SLICE_LITERAL("a/b/main.c")), ".c"),
      "Failed .c");
  asrt_msg(slice_equals_cstr(path_get_extension(SLICE_LITERAL(".config")),
                             ".config"),
           "Failed .config");
  asrt_msg(
      slice_equals_cstr(path_get_extension(SLICE_LITERAL("a/b/.config")), ""),
      "Failed path/.config");
}

/**
 * @brief 测试 2：(单元测试) `path_join`
 */
static void test_path_join(void) {
  printf("--- Test: test_path_join ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);
  string_t s;
  string_init(&s, &alc, 0);

#define CHECK_JOIN(base, part, expected)                                       \
  string_clear(&s);                                                            \
  path_join(&s, SLICE_LITERAL(base), SLICE_LITERAL(part));                     \
  check_str(&s, expected)

// (我们需要 string.h 和 asrt.h)
#define check_str(s, expected)                                                 \
  asrt_msg(strcmp(string_as_cstr(s), expected) == 0, "Join failed")

  CHECK_JOIN("a", "b", "a/b");
  CHECK_JOIN("a/", "b", "a/b");
  CHECK_JOIN("a", "/b", "a/b");
  CHECK_JOIN("a/", "/b", "a/b");
  CHECK_JOIN("", "b", "b");
  CHECK_JOIN("a", "", "a");

  string_destroy(&s);
  bump_destroy(&arena);
}

/**
 * @brief 测试 3：(集成测试) `dir_walk`
 */

// `dir_walk` 的回调函数
static void dir_walk_test_cb(const char *full_path, dir_entry_type_t type,
                             void *userdata) {
  (void)type;
  vec_t *results = (vec_t *)userdata;
  // (我们*知道* full_path 是在 Arena 上的，所以我们可以安全地 push)
  vec_push(results, (void *)full_path);
}

static void test_dir_walk(void) {
  printf("--- Test: test_dir_walk ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  vec_t results; // `Vec<const char*>`
  vec_init(&results, &alc, 0);

  // 1. 创建一个假的目录结构
  // (POSIX: 0755 是 rwxr-xr-x)
  mkdir("fluf_test_dir", 0755);
  mkdir("fluf_test_dir/sub", 0755);
  write_file_bytes("fluf_test_dir/file1.txt", "test", 4);
  write_file_bytes("fluf_test_dir/sub/file2.txt", "test", 4);

  // 2. 遍历!
  dir_walk(&alc, "fluf_test_dir", dir_walk_test_cb, &results);

  // 3. 验证 (我们期望 3 个条目: "sub", "file1.txt", "file2.txt")
  // (注意：`readdir` 不保证顺序！)
  asrt_msg(vec_count(&results) == 3, "dir_walk should find 3 entries");

  // (我们将它们排序以便稳定测试)
  // (这需要一个简单的 `vec_sort`... 暂时我们先线性查找)
  bool found_sub = false;
  bool found_f1 = false;
  bool found_f2 = false;
  for (size_t i = 0; i < vec_count(&results); i++) {
    const char *path = vec_get(&results, i);
    if (strcmp(path, "fluf_test_dir/sub") == 0)
      found_sub = true;
    if (strcmp(path, "fluf_test_dir/file1.txt") == 0)
      found_f1 = true;
    if (strcmp(path, "fluf_test_dir/sub/file2.txt") == 0)
      found_f2 = true;
  }
  asrt_msg(found_sub && found_f1 && found_f2, "Did not find all entries");

  // 4. 清理 (必须按此顺序)
  remove("fluf_test_dir/sub/file2.txt");
  remove("fluf_test_dir/file1.txt");
  rmdir("fluf_test_dir/sub");
  rmdir("fluf_test_dir");

  vec_destroy(&results);
  bump_destroy(&arena);
}

/**
 * 测试运行器 (Test Runner)
 */
int main(void) {
#ifdef NDEBUG
  fprintf(stderr, "Error: Cannot run tests with NDEBUG defined. Recompile in "
                  "Debug mode.\n");
  return 1;
#endif

  printf("=== [fluf] Running tests for <std/fs> ===\n");

  test_path_extension();
  test_path_join();
  test_dir_walk();

  printf("=========================================\n");
  printf("✅ All <std/fs> tests passed!\n");
  printf("=========================================\n");

  return 0;
}