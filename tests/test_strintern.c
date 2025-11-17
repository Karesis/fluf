#include <stdio.h>
#include <string.h>

// --- Fluf 依赖 ---
#include <assert.h>
#include <core/mem/allocer.h> // 你的路径
#include <core/msg/asrt.h>
#include <core/msg/dbg.h>
#include <std/allocer/bump/bump.h> // 你的路径
#include <std/allocer/bump/glue.h> // 你的路径
#include <std/string/strslice.h>   // 你的路径

// --- 我们要测试的模块 ---
#include <std/string/strintern.h>

/**
 * 测试 1：基本 C-String 驻留
 */
static void test_intern_cstr(void) {
  printf("--- Test: test_intern_cstr ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  strintern_t interner;                         // 在栈上
  bool ok = strintern_init(&interner, &alc, 0); // 使用 _init
  asrt_msg(ok, "strintern_init failed");

  // 1. 驻留
  const char *s1 = strintern_intern_cstr(&interner, "hello");
  asrt_msg(s1 != NULL, "intern_cstr failed (OOM?)");

  // 2. 验证 \0 终止符
  asrt_msg(strcmp(s1, "hello") == 0, "Returned string is not a valid C-String");

  // 3. 验证 Count
  asrt_msg(strintern_count(&interner) == 1, "Count should be 1");

  strintern_destroy(&interner); // 使用 _destroy
  bump_destroy(&arena);
}

/**
 * 测试 2：指针恒等性 (Uniqueness)
 */
static void test_intern_uniqueness(void) {
  printf("--- Test: test_intern_uniqueness ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  strintern_t interner; // 在栈上
  bool ok = strintern_init(&interner, &alc, 0);
  asrt_msg(ok, "strintern_init failed");

  // 1. 测试 C-String
  const char *s1 = strintern_intern_cstr(&interner, "unique");
  const char *s2 = strintern_intern_cstr(&interner, "unique");

  asrt_msg(s1 == s2, "Pointers MUST be identical for C-String (s1 == s2)");
  asrt_msg(strintern_count(&interner) == 1,
           "Count should be 1 after duplicate C-String");

  // 2. 测试 Slice
  strslice_t slice = SLICE_LITERAL("slice");
  const char *s3 = strintern_intern_slice(&interner, slice);
  const char *s4 = strintern_intern_slice(&interner, slice);

  asrt_msg(s3 == s4, "Pointers MUST be identical for slice (s3 == s4)");
  asrt_msg(strintern_count(&interner) == 2,
           "Count should be 2 after new slice");

  // 3. 测试混合 (C-String vs Slice)
  const char *s5 = strintern_intern_cstr(&interner, "slice");
  asrt_msg(s3 == s5, "Pointers MUST be identical when mixing cstr and slice");
  asrt_msg(strintern_count(&interner) == 2, "Count should still be 2");

  strintern_destroy(&interner);
  bump_destroy(&arena);
}

/**
 * 测试 3：非 \0 结尾的切片 (词法分析器模拟)
 */
static void test_intern_non_null_terminated(void) {
  printf("--- Test: test_intern_non_null_terminated ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  strintern_t interner; // 在栈上
  bool ok = strintern_init(&interner, &alc, 0);
  asrt_msg(ok, "strintern_init failed");

  const char *source_text = "let x = 10;let"; // 源代码

  strslice_t s_let = {.ptr = &source_text[0], .len = 3};
  strslice_t s_x = {.ptr = &source_text[4], .len = 1};

  const char *c_let = strintern_intern_slice(&interner, s_let);
  const char *c_x = strintern_intern_slice(&interner, s_x);

  asrt_msg(c_let != s_let.ptr, "Pointer MUST be a copy, not the original");
  asrt_msg(strcmp(c_let, "let") == 0,
           "Returned string 'let' failed strcmp (bad \\0)");

  asrt_msg(c_x != s_x.ptr, "Pointer 'x' MUST be a copy");
  asrt_msg(strcmp(c_x, "x") == 0,
           "Returned string 'x' failed strcmp (bad \\0)");

  asrt_msg(strintern_count(&interner) == 2, "Count should be 2");

  const char *c_let_2 = strintern_intern_slice(&interner, SLICE_LITERAL("let"));
  asrt_msg(c_let == c_let_2, "Uniqueness failed for non-null-terminated slice");

  strintern_destroy(&interner);
  bump_destroy(&arena);
}

/**
 * 测试 4：扩容 (Growth / Resize)
 */
static void test_intern_growth(void) {
  printf("--- Test: test_intern_growth ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  strintern_t interner;                         // 在栈上
  bool ok = strintern_init(&interner, &alc, 4); // 强制扩容
  asrt_msg(ok, "strintern_init failed");

  const char *s1 = strintern_intern_cstr(&interner, "s1");
  const char *s2 = strintern_intern_cstr(&interner, "s2");
  const char *s3 = strintern_intern_cstr(&interner, "s3");
  asrt_msg(strintern_count(&interner) == 3, "Count should be 3 before resize");

  const char *s4 = strintern_intern_cstr(&interner, "s4");
  asrt_msg(strintern_count(&interner) == 4, "Count should be 4 after resize");

  asrt_msg(strcmp(s1, "s1") == 0, "Resize corrupted s1");
  asrt_msg(strcmp(s2, "s2") == 0, "Resize corrupted s2");
  asrt_msg(strcmp(s3, "s3") == 0, "Resize corrupted s3");
  asrt_msg(strcmp(s4, "s4") == 0, "Resize corrupted s4");

  const char *s1_again = strintern_intern_cstr(&interner, "s1");
  asrt_msg(s1 == s1_again, "Find failed after resize (s1)");
  const char *s3_again = strintern_intern_cstr(&interner, "s3");
  asrt_msg(s3 == s3_again, "Find failed after resize (s3)");

  strintern_destroy(&interner);
  bump_destroy(&arena);
}

/**
 * 测试 5：`clear` (编译器循环模拟)
 */
static void test_intern_clear(void) {
  printf("--- Test: test_intern_clear ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  strintern_t interner; // 在栈上
  bool ok = strintern_init(&interner, &alc, 0);
  asrt_msg(ok, "strintern_init failed");

  // 1. 编译运行 #1
  const char *s1 = strintern_intern_cstr(&interner, "foo");
  asrt_msg(strintern_count(&interner) == 1, "Count should be 1");

  // 2. 编译结束
  strintern_clear(&interner); // 清空 interner 哈希表
  bump_reset(&arena);         // *释放* "foo" 的内存
                              // `interner` 在栈上，是安全的

  asrt_msg(strintern_count(&interner) == 0, "Count should be 0 after clear");

  // 3. 编译运行 #2
  const char *s2 = strintern_intern_cstr(&interner, "foo");

  // (移除了调试用的 printf/dbg/assert)
  asrt_msg(strintern_count(&interner) == 1,
           "Count should be 1 after re-intern");

  // 4. 关键：s1 的内存已被回收，s2 是一个 *新* 指针
  asrt_msg(s1 != s2, "Pointers MUST be different after arena reset");
  asrt_msg(strcmp(s2, "foo") == 0, "s2 should be valid");

  strintern_destroy(&interner);
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

  printf("=== [fluf] Running tests for <std/strintern> ===\n");

  test_intern_cstr();
  test_intern_uniqueness();
  test_intern_non_null_terminated();
  test_intern_growth();
  test_intern_clear();

  printf("==============================================\n");
  printf("✅ All <std/strintern> tests passed!\n");
  printf("==============================================\n");

  return 0;
}