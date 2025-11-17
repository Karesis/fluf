#include <stdio.h>
#include <string.h>

// --- Fluf 依赖 ---
#include <core/mem/allocer.h> // 抽象 Allocer
#include <core/msg/asrt.h>
#include <std/allocer/bump/bump.h> // 具体 Bump 实现
#include <std/allocer/bump/glue.h> // "胶水" (bump -> allocer)
#include <std/string/str_slice.h>  // 字符串切片

// --- 我们要测试的模块 ---
#include <std/string/string.h> // 你的路径

/**
 * @brief 辅助函数，用于验证 string_t 的内部状态
 */
static void check_str(const string_t *s, const char *expected) {
  size_t expected_len = strlen(expected);

  // 1. 验证 C-String (strcmp 检查 \0 终止符)
  asrt_msg(strcmp(string_as_cstr(s), expected) == 0,
           "C-String content mismatch");

  // 2. 验证 Count
  asrt_msg(string_count(s) == expected_len, "String count mismatch");

  // 3. 验证 Slice
  strslice_t slice = string_as_slice(s);
  asrt_msg(slice.len == expected_len, "Slice length mismatch");
  asrt_msg(slice_equals_cstr(slice, expected), "Slice content mismatch");
}

/**
 * 测试 1：基本操作 (Init, Push, CStr)
 */
static void test_init_push_cstr(void) {
  printf("--- Test: test_init_push_cstr ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  // 1. 初始化
  string_t s;
  bool ok = string_init(&s, &alc, 0); // 0 = 默认容量
  asrt_msg(ok, "string_init failed");
  check_str(&s, ""); // 初始状态应为空 C-String

  // 2. Push char
  ok = string_push(&s, 'f');
  asrt_msg(ok, "push failed");
  ok = string_push(&s, 'l');
  asrt_msg(ok, "push failed");
  ok = string_push(&s, 'u');
  asrt_msg(ok, "push failed");
  ok = string_push(&s, 'f');
  asrt_msg(ok, "push failed");
  check_str(&s, "fluf");

  // 3. 销毁
  string_destroy(&s);
  bump_destroy(&arena);
}

/**
 * 测试 2：Append (CStr 和 Slice)
 */
static void test_append(void) {
  printf("--- Test: test_append ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);
  string_t s;
  string_init(&s, &alc, 0);

  // 1. Append CStr
  bool ok = string_append_cstr(&s, "Hello");
  asrt_msg(ok, "append_cstr failed");
  check_str(&s, "Hello");

  // 2. Append Slice (非 \0 结尾)
  const char *source = " World! (extra)";
  strslice_t slice = {.ptr = source, .len = 7}; // " World!"

  ok = string_append_slice(&s, slice);
  asrt_msg(ok, "append_slice failed");
  check_str(&s, "Hello World!");

  // 3. Append CStr again
  ok = string_append_cstr(&s, " Goodbye.");
  asrt_msg(ok, "append_cstr (2) failed");
  check_str(&s, "Hello World! Goodbye.");

  string_destroy(&s);
  bump_destroy(&arena);
}

/**
 * 测试 3：扩容 (Growth / Resize)
 */
static void test_growth(void) {
  printf("--- Test: test_growth ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  // 1. 初始化一个小容量 (2) 来强制扩容
  // STRING_DEFAULT_CAPACITY 默认是 8, 我们用 2
  string_t s;
  string_init(&s, &alc, 2);
  asrt_msg(s.capacity == 2, "Initial capacity was not 2");

  // 2. 填满容量
  string_push(&s, 'a');
  string_push(&s, 'b');
  check_str(&s, "ab");
  asrt_msg(s.count == 2, "Count should be 2 (full)");

  // 3. 触发扩容 (push)
  string_push(&s, 'c');
  check_str(&s, "abc");
  asrt_msg(s.capacity >= 3, "Capacity did not increase on push"); // 应该是 4

  // 4. 触发扩容 (append_cstr)
  // (容量是 4, count 是 3, 剩余 1)
  string_append_cstr(&s, "def"); // 需要 3, 触发扩容
  check_str(&s, "abcdef");
  asrt_msg(s.capacity >= 6,
           "Capacity did not increase on append_cstr"); // 应该是 8

  // 5. 触发扩容 (append_slice)
  // (容量是 8, count 是 6, 剩余 2)
  strslice_t slice = SLICE_LITERAL("ghijkl"); // 需要 6, 触发扩容
  string_append_slice(&s, slice);
  check_str(&s, "abcdefghijkl");
  asrt_msg(s.capacity >= 12,
           "Capacity did not increase on append_slice"); // 应该是 16

  string_destroy(&s);
  bump_destroy(&arena);
}

/**
 * 测试 4：`clear`
 */
static void test_clear(void) {
  printf("--- Test: test_clear ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);
  string_t s;
  string_init(&s, &alc, 0);

  string_append_cstr(&s, "some data");
  check_str(&s, "some data");
  size_t old_capacity = s.capacity;
  asrt_msg(old_capacity > 0, "Capacity should be > 0");

  // 1. 清除
  string_clear(&s);
  check_str(&s, ""); // 验证 \0 终止符和 count
  asrt_msg(s.capacity == old_capacity,
           "Capacity should be retained after clear");

  // 2. 重用
  string_append_cstr(&s, "new data");
  check_str(&s, "new data");

  string_destroy(&s);
  bump_destroy(&arena);
}

/**
 * 测试 5：格式化 (Fmt)
 */
static void test_format(void) {
  printf("--- Test: test_format ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);
  string_t s;
  string_init(&s, &alc, 0);

  // 1. 简单格式化
  bool ok = string_append_fmt(&s, "User: %s, ID: %d", "karesis", 123);
  asrt_msg(ok, "append_fmt failed");
  check_str(&s, "User: karesis, ID: 123");

  // 2. 格式化追加 (append)
  ok = string_append_fmt(&s, " | Status: %.1f%%", 99.9);
  asrt_msg(ok, "append_fmt (2) failed");
  check_str(&s, "User: karesis, ID: 123 | Status: 99.9%");

  // 3. 格式化触发扩容
  // (容量现在 > 35, 让我们追加一个 50 字节的字符串)
  const char *long_str = "This is a very long string that will force a resize";
  ok = string_append_fmt(&s, " | Extra: %s", long_str);
  asrt_msg(ok, "append_fmt (growth) failed");

  // 验证旧内容
  asrt_msg(strncmp(string_as_cstr(&s), "User: karesis, ID: 123", 22) == 0,
           "Content corrupted after fmt growth");
  // 验证总长度
  asrt_msg(string_count(&s) > 80, "Count is wrong after fmt growth");

  string_destroy(&s);
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

  printf("=== [fluf] Running tests for <std/string> ===\n");

  test_init_push_cstr();
  test_append();
  test_growth();
  test_clear();
  test_format();

  printf("==============================================\n");
  printf("✅ All <std/string> tests passed!\n");
  printf("==============================================\n");

  return 0;
}