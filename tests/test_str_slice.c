#include <stdio.h>
#include <string.h>

// --- Fluf 依赖 ---
#include <core/msg/asrt.h>

// --- 我们要测试的模块 ---
#include <std/string/str_slice.h> // 你的路径

/**
 * @brief 测试 1：构造函数和比较函数
 */
static void test_construct_and_compare(void) {
  printf("--- Test: test_construct_and_compare ---\n");

  strslice_t s1 = SLICE_LITERAL("hello");
  asrt_msg(s1.len == 5, "SLICE_LITERAL len failed");
  strslice_t s2 = slice_from_cstr("hello");
  asrt_msg(s2.len == 5, "slice_from_cstr len failed");
  strslice_t s3 = SLICE_LITERAL("world");

  // 比较
  asrt_msg(slice_equals(s1, s2), "slice_equals failed");
  asrt_msg(!slice_equals(s1, s3), "slice_equals (negative) failed");

  // 【【【 修复 】】】
  // 使用 slice_equals_cstr，这是正确的函数
  asrt_msg(slice_equals_cstr(s1, "hello"), "slice_equals_cstr failed");
  asrt_msg(!slice_equals_cstr(s1, "hell"), "slice_equals_cstr (len) failed");
  asrt_msg(!slice_equals_cstr(s1, "world"),
           "slice_equals_cstr (content) failed");
}

/**
 * @brief 测试 2：前缀和后缀
 */
static void test_prefix_suffix(void) {
  printf("--- Test: test_prefix_suffix ---\n");

  strslice_t s = SLICE_LITERAL("hello_world");

  // 1. 前缀 (Starts With)
  asrt_msg(slice_starts_with(s, SLICE_LITERAL("hello")),
           "slice_starts_with (slice) failed");
  // 【【【 修复 】】】
  asrt_msg(slice_starts_with_cstr(s, "hello"), "slice_starts_with_cstr failed");
  asrt_msg(!slice_starts_with_cstr(s, "world"),
           "slice_starts_with_cstr (negative) failed");
  asrt_msg(!slice_starts_with_cstr(s, "hello_world_long"),
           "slice_starts_with_cstr (too long) failed");

  // 2. 后缀 (Ends With)
  asrt_msg(slice_ends_with(s, SLICE_LITERAL("world")),
           "slice_ends_with (slice) failed");
  // 【【【 新增测试 】】】
  asrt_msg(slice_ends_with_cstr(s, "world"), "slice_ends_with_cstr failed");
  asrt_msg(!slice_ends_with_cstr(s, "hello"),
           "slice_ends_with_cstr (negative) failed");
  asrt_msg(!slice_ends_with_cstr(s, "long_hello_world"),
           "slice_ends_with_cstr (too long) failed");
}

/**
 * 测试 3：修剪 (Trim)
 */
static void test_trim(void) {
  printf("--- Test: test_trim ---\n");

  strslice_t s1 = SLICE_LITERAL(" \t \n hello world \r \n ");
  strslice_t t1 = slice_trim(s1);
  asrt_msg(slice_equals_cstr(t1, "hello world"),
           "slice_trim failed"); // <-- 改用 _cstr

  strslice_t s2 = SLICE_LITERAL("  hello");
  strslice_t t2 = slice_trim(s2);
  asrt_msg(slice_equals_cstr(t2, "hello"), "slice_trim (left) failed");

  strslice_t s3 = SLICE_LITERAL("hello  ");
  strslice_t t3 = slice_trim(s3);
  asrt_msg(slice_equals_cstr(t3, "hello"), "slice_trim (right) failed");

  strslice_t s4 = SLICE_LITERAL("hello");
  strslice_t t4 = slice_trim(s4);
  asrt_msg(slice_equals_cstr(t4, "hello"), "slice_trim (no-op) failed");
  asrt_msg(t4.ptr == s4.ptr, "slice_trim (no-op) should not change ptr");

  strslice_t s5 = SLICE_LITERAL(" \t \n ");
  strslice_t t5 = slice_trim(s5);
  asrt_msg(t5.len == 0, "slice_trim (all whitespace) len failed");

  strslice_t s6 = SLICE_LITERAL("");
  strslice_t t6 = slice_trim(s6);
  asrt_msg(t6.len == 0, "slice_trim (empty) len failed");
}

/**
 * 测试 4：分割 (Split)
 */
static void test_split(void) {
  printf("--- Test: test_split ---\n");
  strslice_t s, token;
  bool ok;

  s = SLICE_LITERAL("a,b,c");
  ok = slice_split_next(&s, ',', &token);
  asrt_msg(ok && slice_equals_cstr(token, "a"),
           "split 1 failed"); // <-- 改用 _cstr
  asrt_msg(slice_equals_cstr(s, "b,c"), "split 1 remaining failed");

  ok = slice_split_next(&s, ',', &token);
  asrt_msg(ok && slice_equals_cstr(token, "b"), "split 2 failed");
  asrt_msg(slice_equals_cstr(s, "c"), "split 2 remaining failed");

  ok = slice_split_next(&s, ',', &token);
  asrt_msg(ok && slice_equals_cstr(token, "c"), "split 3 failed");
  asrt_msg(s.ptr == NULL && s.len == 0,
           "split 3 remaining failed (should be sentinel)");

  ok = slice_split_next(&s, ',', &token);
  asrt_msg(!ok, "split 4 should return false (finished)");

  // (其他边缘情况测试是正确的)
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

  printf("=== [fluf] Running tests for <std/string/str_slice> ===\n");

  test_construct_and_compare();
  test_prefix_suffix();
  test_trim();
  test_split();

  printf("=====================================================\n");
  printf("✅ All <std/string/str_slice> tests passed!\n");
  printf("=====================================================\n");

  return 0;
}