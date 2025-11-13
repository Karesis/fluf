#include <stdio.h>
#include <string.h>

// --- Fluf 依赖 ---
#include <core/msg/asrt.h>
#include <core/mem/allocer.h>      // 抽象 Allocer
#include <std/allocer/bump/bump.h> // 具体 Bump 实现
#include <std/allocer/bump/glue.h> // "胶水" (bump -> allocer)
#include <core/span.h>             // 包含 span_t

// --- 我们要测试的模块 ---
#include <std/io/sourcemap.h> // 你的路径

/**
 * @brief 辅助函数，用于验证 `source_loc_t` 的值
 */
static void
check_loc(source_loc_t loc, const char *file, size_t line, size_t col)
{
  asrt_msg(strcmp(loc.filename, file) == 0, "Filename mismatch");
  asrt_msg(loc.line == line, "Line number mismatch");
  asrt_msg(loc.column == col, "Column number mismatch");
}

/**
 * 测试 1：单个文件的基本查找
 */
static void
test_simple_lookup(void)
{
  printf("--- Test: test_simple_lookup ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  sourcemap_t map;
  bool ok = sourcemap_init(&map, &alc);
  asrt_msg(ok, "sourcemap_init failed");

  // 01234 5 67890 1 23456
  // line1 \n line2 \n line3
  const char *content = "line1\nline2\nline3";
  str_slice_t slice = slice_from_cstr(content);

  size_t file_id = sourcemap_add_file(&map, "test.nyan", slice);
  asrt_msg(file_id == 0, "FileID should be 0");

  source_loc_t loc;

  // 1. "l" (line1)
  ok = sourcemap_lookup(&map, 0, &loc);
  asrt_msg(ok, "Lookup failed at offset 0");
  check_loc(loc, "test.nyan", 1, 1);

  // 2. "1" (line1)
  ok = sourcemap_lookup(&map, 4, &loc);
  asrt_msg(ok, "Lookup failed at offset 4");
  check_loc(loc, "test.nyan", 1, 5);

  // 3. "\n" (第一个)
  ok = sourcemap_lookup(&map, 5, &loc);
  asrt_msg(ok, "Lookup failed at offset 5");
  check_loc(loc, "test.nyan", 1, 6);

  // 4. "l" (line2)
  ok = sourcemap_lookup(&map, 6, &loc);
  asrt_msg(ok, "Lookup failed at offset 6");
  check_loc(loc, "test.nyan", 2, 1);

  // 5. "2" (line2)
  ok = sourcemap_lookup(&map, 10, &loc);
  asrt_msg(ok, "Lookup failed at offset 10");
  check_loc(loc, "test.nyan", 2, 5);

  // 6. "3" (line3)
  ok = sourcemap_lookup(&map, 16, &loc);
  asrt_msg(ok, "Lookup failed at offset 16");
  check_loc(loc, "test.nyan", 3, 5);

  // 7. 越界 (最后一个字符是 offset 16, 长度是 17)
  ok = sourcemap_lookup(&map, 17, &loc);
  asrt_msg(!ok, "Lookup should fail at offset 17 (EOF)");
  ok = sourcemap_lookup(&map, 999, &loc);
  asrt_msg(!ok, "Lookup should fail at offset 999 (OOB)");

  sourcemap_destroy(&map);
  bump_destroy(&arena);
}

/**
 * 测试 2：多文件查找
 */
static void
test_multiple_files(void)
{
  printf("--- Test: test_multiple_files ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  sourcemap_t map;
  sourcemap_init(&map, &alc);

  // 01 2 34
  // a\nb c
  // (len=5, base_offset=0)
  size_t f1 = sourcemap_add_file(&map, "file1.nyan", SLICE_LITERAL("a\nb c"));

  // 01 2 34
  // x\ny z
  // (len=5, base_offset=5)
  size_t f2 = sourcemap_add_file(&map, "file2.nyan", SLICE_LITERAL("x\ny z"));

  asrt_msg(f1 == 0, "FileID 1 should be 0");
  asrt_msg(f2 == 1, "FileID 2 should be 1");

  source_loc_t loc;
  bool ok;

  // --- File 1 ---
  // 1. "a" (f1)
  ok = sourcemap_lookup(&map, 0, &loc);
  asrt_msg(ok, "Lookup failed at offset 0 (f1)");
  check_loc(loc, "file1.nyan", 1, 1);

  // 2. "b" (f1)
  ok = sourcemap_lookup(&map, 2, &loc);
  asrt_msg(ok, "Lookup failed at offset 2 (f1)");
  check_loc(loc, "file1.nyan", 2, 1);

  // 3. "c" (f1)
  ok = sourcemap_lookup(&map, 4, &loc);
  asrt_msg(ok, "Lookup failed at offset 4 (f1)");
  check_loc(loc, "file1.nyan", 2, 3);

  // --- File 2 (关键测试) ---
  // 4. "x" (f2)
  ok = sourcemap_lookup(&map, 5, &loc); // 5 = base_offset
  asrt_msg(ok, "Lookup failed at offset 5 (f2)");
  check_loc(loc, "file2.nyan", 1, 1);

  // 5. "y" (f2)
  ok = sourcemap_lookup(&map, 7, &loc); // 7 = base 5 + local 2
  asrt_msg(ok, "Lookup failed at offset 7 (f2)");
  check_loc(loc, "file2.nyan", 2, 1);

  // 6. 越界
  ok = sourcemap_lookup(&map, 10, &loc); // total_size = 10
  asrt_msg(!ok, "Lookup should fail at offset 10 (EOF)");

  sourcemap_destroy(&map);
  bump_destroy(&arena);
}

/**
 * 测试 3：边缘情况 (空文件, 只有换行)
 */
static void
test_edge_cases(void)
{
  printf("--- Test: test_edge_cases ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  sourcemap_t map;
  sourcemap_init(&map, &alc);

  // 1. 空文件 (len=0)
  size_t f1 = sourcemap_add_file(&map, "empty.nyan", SLICE_LITERAL(""));
  asrt_msg(f1 == 0, "Adding empty file failed");

  // 2. 只有换行 (len=1)
  size_t f2 = sourcemap_add_file(&map, "newline.nyan", SLICE_LITERAL("\n"));
  (void)f2;

  // 3. 查找 (空文件)
  source_loc_t loc;
  bool ok = sourcemap_lookup(&map, 0, &loc);
  // (offset 0 应该属于 file 2)
  check_loc(loc, "newline.nyan", 1, 1);

  // 4. 查找 (newline.nyan)
  ok = sourcemap_lookup(&map, 0, &loc);
  asrt_msg(ok, "Lookup failed at offset 0 (newline)");
  check_loc(loc, "newline.nyan", 1, 1);

  // 越界
  ok = sourcemap_lookup(&map, 1, &loc);
  asrt_msg(!ok, "Lookup should fail at offset 1 (EOF)");

  sourcemap_destroy(&map);
  bump_destroy(&arena);
}

/**
 * 测试 4：测试 span_t (来自 core/span.h)
 */
static void
test_span_helpers(void)
{
  printf("--- Test: test_span_helpers ---\n");

  // 1. range / len
  span_t s1 = range(10, 20);
  asrt_msg(s1.start == 10, "span_from_range start failed");
  asrt_msg(s1.end == 20, "span_from_range end failed");
  asrt_msg(span_len(s1) == 10, "span_len failed");

  // 2. len
  span_t s2 = span_from_len(10, 5);
  asrt_msg(s2.start == 10, "span_from_len start failed");
  asrt_msg(s2.end == 15, "span_from_len end failed");

  // 3. merge
  span_t s3 = span_merge(s1, s2); // (10-20) + (10-15)
  asrt_msg(s3.start == 10, "merge failed (start)");
  asrt_msg(s3.end == 20, "merge failed (end)");

  span_t s4 = range(30, 40);
  span_t s5 = span_merge(s1, s4); // (10-20) + (30-40)
  asrt_msg(s5.start == 10, "merge (2) failed (start)");
  asrt_msg(s5.end == 40, "merge (2) failed (end)");
}

/**
 * 测试运行器 (Test Runner)
 */
int main(void)
{
#ifdef NDEBUG
  fprintf(stderr, "Error: Cannot run tests with NDEBUG defined. Recompile in Debug mode.\n");
  return 1;
#endif

  printf("=== [fluf] Running tests for <std/diag/sourcemap> ===\n");

  test_simple_lookup();
  test_multiple_files();
  test_edge_cases();
  test_span_helpers(); // (这个测试不依赖 sourcemap，只是测试 span.h)

  printf("=====================================================\n");
  printf("✅ All <std/diag/sourcemap> tests passed!\n");
  printf("=====================================================\n");

  return 0;
}