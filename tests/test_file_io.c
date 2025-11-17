#include <stdio.h>  // C 标准 I/O，我们*只*需要它来进行 `remove()`
#include <string.h> // for strlen, strcmp

// --- Fluf 依赖 ---
#include <core/mem/allocer.h> // 抽象 Allocer
#include <core/msg/asrt.h>
#include <std/allocer/bump/bump.h> // 具体 Bump 实现
#include <std/allocer/bump/glue.h> // "胶水" (bump -> allocer)
#include <std/string/str_slice.h>  // 字符串切片

// --- 我们要测试的模块 ---
#include <std/io/file.h> // 你的路径

/**
 * @brief 我们将用于测试的临时文件名。
 * (它会出现在你运行 `make test` 的根目录)
 */
static const char *TEST_FILE_PATH = "fluf_io_test.tmp";

/**
 * @brief 辅助函数：确保测试文件被删除
 */
static void cleanup_test_file(void) { remove(TEST_FILE_PATH); }

/**
 * @brief 测试 1：写入/读取 "往返" (Roundtrip) - 核心测试
 */
static void test_write_read_roundtrip(void) {
  printf("--- Test: test_write_read_roundtrip ---\n");

  // --- 设置 Allocator ---
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  // --- 1. 定义测试内容 ---
  // (使用一个包含换行符的复杂字符串)
  const char *test_content = "Hello Fluf!\nThis is line 2.\nAnd line 3.";
  size_t test_len = strlen(test_content);

  // --- 2. 测试写入 ---
  // (在测试开始前先清理一次，确保是新文件)
  cleanup_test_file();
  bool ok = write_file_bytes(TEST_FILE_PATH, test_content, test_len);
  asrt_msg(ok, "write_file_bytes failed (check permissions?)");

  // --- 3. 测试读取 ---
  strslice_t read_slice;
  ok = read_file_to_slice(&alc, TEST_FILE_PATH, &read_slice);
  asrt_msg(ok, "read_file_to_slice failed (file not found?)");

  // --- 4. 验证内容 ---

  // a) 验证长度 (不包含 \0)
  asrt_msg(read_slice.len == test_len,
           "Read length does not match written length");

  // b) 验证内容 (使用 slice_equals_cstr)
  asrt_msg(slice_equals_cstr(read_slice, test_content),
           "Read content does not match written content");

  // c) 验证 \0 终止符 (read_file_to_slice 的核心特性)
  //    (我们直接使用 strcmp，它依赖 \0)
  asrt_msg(strcmp(read_slice.ptr, test_content) == 0,
           "Read slice is not a valid C-string (missing \\0?)");

  // --- 5. 清理 ---
  cleanup_test_file();
  bump_destroy(&arena);
}

/**
 * @brief 测试 2：读取不存在的文件
 */
static void test_read_non_existent(void) {
  printf("--- Test: test_read_non_existent ---\n");

  // --- 设置 Allocator ---
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  // --- 确保文件 *真的* 不存在 ---
  const char *no_such_file = "fluf_no_such_file_12345.tmp";
  remove(no_such_file);

  // --- 测试读取 ---
  strslice_t read_slice;
  bool ok = read_file_to_slice(&alc, no_such_file, &read_slice);

  // --- 验证 ---
  asrt_msg(ok == false,
           "read_file_to_slice should return 'false' for non-existent file");

  // --- 清理 ---
  bump_destroy(&arena);
}

/**
 * 测试 3：写入空文件
 */
static void test_write_empty(void) {
  printf("--- Test: test_write_empty ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  // 1. 写入 0 字节
  cleanup_test_file();
  bool ok = write_file_bytes(TEST_FILE_PATH, "dont-write-this", 0);
  asrt_msg(ok, "write_file_bytes (empty) failed");

  // 2. 读回
  strslice_t read_slice;
  ok = read_file_to_slice(&alc, TEST_FILE_PATH, &read_slice);
  asrt_msg(ok, "read_file_to_slice (empty) failed");

  // 3. 验证 (0 字节)
  asrt_msg(read_slice.len == 0, "Length of empty file should be 0");
  asrt_msg(read_slice.ptr[0] == '\0',
           "Empty file read should still be null-terminated");

  // 4. 清理
  cleanup_test_file();
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

  printf("=== [fluf] Running tests for <std/io/file> ===\n");

  test_write_read_roundtrip();
  test_read_non_existent();
  test_write_empty();

  printf("==============================================\n");
  printf("✅ All <std/io/file> tests passed!\n");
  printf("==============================================\n");

  return 0;
}