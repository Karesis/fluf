#include <stdio.h>
#include <string.h>

// --- Fluf 依赖 ---
#include <core/msg/asrt.h>
#include <std/string/strslice.h>

// --- 我们要测试的模块 ---
#include <std/string/chars.h>

/**
 * @brief 测试 1：初始化、EOF 和空字符串
 */
static void test_init_and_eof(void) {
  printf("--- Test: test_init_and_eof ---\n");

  strslice_t s = SLICE_LITERAL("");
  chars_t iter;
  utf8_t c;

  // 1. 初始化
  chars_init(&iter, s);
  asrt_msg(iter.offset == 0, "Initial offset should be 0");
  asrt_msg(iter.slice.ptr == s.ptr && iter.slice.len == s.len,
           "Slice was not copied");

  // 2. 在空字符串上检查 EOF
  asrt_msg(chars_is_eof(&iter), "chars_is_eof should be true for empty string");
  asrt_msg(chars_offset(&iter) == 0, "Offset should still be 0");

  // 3. Peek 和 Consume 应该失败
  asrt_msg(!chars_peek(&iter, &c), "Peek on empty string should return false");
  asrt_msg(!chars_consume(&iter, &c),
           "Consume on empty string should return false");
  asrt_msg(!chars_advance(&iter),
           "Advance on empty string should return false");
}

/**
 * @brief 测试 2：纯 ASCII 字符串 (快速路径)
 */
static void test_ascii_string(void) {
  printf("--- Test: test_ascii_string ---\n");

  strslice_t s = SLICE_LITERAL("Hi");
  chars_t iter;
  utf8_t c;
  bool ok;

  chars_init(&iter, s);

  // 1. 检查 'H'
  asrt_msg(!chars_is_eof(&iter), "Should not be EOF");
  asrt_msg(chars_offset(&iter) == 0, "Offset should be 0");

  ok = chars_peek(&iter, &c);
  asrt_msg(ok, "Peek failed");
  asrt_msg(c.codepoint == 'H' && c.width == 1, "Peek 'H' failed");
  asrt_msg(chars_offset(&iter) == 0, "Offset should not change after peek");

  ok = chars_consume(&iter, &c);
  asrt_msg(ok, "Consume failed");
  asrt_msg(c.codepoint == 'H' && c.width == 1, "Consume 'H' failed");
  asrt_msg(chars_offset(&iter) == 1, "Offset should be 1 after consume");

  // 2. 检查 'i' (使用 advance)
  asrt_msg(!chars_is_eof(&iter), "Should not be EOF");
  ok = chars_peek(&iter, &c);
  asrt_msg(ok && c.codepoint == 'i' && c.width == 1, "Peek 'i' failed");

  ok = chars_advance(&iter);
  asrt_msg(ok, "Advance 'i' failed");
  asrt_msg(chars_offset(&iter) == 2, "Offset should be 2 after advance");

  // 3. 检查 EOF
  asrt_msg(chars_is_eof(&iter), "Should be EOF");
  asrt_msg(!chars_peek(&iter, &c), "Peek at EOF should fail");
  asrt_msg(!chars_consume(&iter, &c), "Consume at EOF should fail");
}

/**
 * @brief 测试 3：多字节和混合字符串 (解码路径)
 */
static void test_multibyte_string(void) {
  printf("--- Test: test_multibyte_string ---\n");

  // "aé中🚀z"
  // a: 1-byte
  // é: 2-bytes (0xC3 0xA9)
  // 中: 3-bytes (0xE4 0xB8 0xAD)
  // 🚀: 4-bytes (0xF0 0x9F 0x9A 0x80)
  // z: 1-byte
  const char *raw = "a\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x9A\x80z";
  strslice_t s = {.ptr = raw, .len = strlen(raw)};
  asrt_msg(s.len == 1 + 2 + 3 + 4 + 1, "Test string length is wrong");

  chars_t iter;
  utf8_t c;
  bool ok;
  chars_init(&iter, s);

  // 1. 'a'
  ok = chars_consume(&iter, &c);
  asrt_msg(ok && c.codepoint == 0x61 && c.width == 1, "Consume 'a' failed");
  asrt_msg(chars_offset(&iter) == 1, "Offset failed (exp 1)");

  // 2. 'é'
  ok = chars_consume(&iter, &c);
  asrt_msg(ok && c.codepoint == 0xE9 && c.width == 2, "Consume 'é' failed");
  asrt_msg(chars_offset(&iter) == 3, "Offset failed (exp 3)");

  // 3. '中'
  ok = chars_consume(&iter, &c);
  asrt_msg(ok && c.codepoint == 0x4E2D && c.width == 3, "Consume '中' failed");
  asrt_msg(chars_offset(&iter) == 6, "Offset failed (exp 6)");

  // 4. '🚀'
  ok = chars_consume(&iter, &c);
  asrt_msg(ok && c.codepoint == 0x1F680 && c.width == 4, "Consume '🚀' failed");
  asrt_msg(chars_offset(&iter) == 10, "Offset failed (exp 10)");

  // 5. 'z'
  ok = chars_consume(&iter, &c);
  asrt_msg(ok && c.codepoint == 0x7A && c.width == 1, "Consume 'z' failed");
  asrt_msg(chars_offset(&iter) == 11, "Offset failed (exp 11)");

  // 6. EOF
  asrt_msg(chars_is_eof(&iter), "Should be EOF at the end");
  asrt_msg(!chars_consume(&iter, &c), "Consume at EOF should fail");
}

/**
 * @brief 测试 4：非法的 UTF-8 序列 (健壮性)
 */
static void test_invalid_utf8(void) {
  printf("--- Test: test_invalid_utf8 ---\n");
  chars_t iter;
  utf8_t c;
  bool ok;

  // 1. 非法的起始字节 (一个单独的 continuation byte)
  strslice_t s1 = SLICE_LITERAL("\x80"); // 10000000
  chars_init(&iter, s1);
  ok = chars_consume(&iter, &c);
  asrt_msg(ok, "Should consume invalid byte");
  asrt_msg(c.codepoint == 0xFFFD && c.width == 1,
           "Invalid start byte should be 0xFFFD(1)");
  asrt_msg(chars_is_eof(&iter), "Should be EOF after invalid byte");

  // 2. 截断的 2-byte 序列
  strslice_t s2 = SLICE_LITERAL("\xC3"); // 11000011
  chars_init(&iter, s2);
  ok = chars_consume(&iter, &c);
  asrt_msg(ok, "Should consume truncated byte");
  asrt_msg(c.codepoint == 0xFFFD && c.width == 1,
           "Truncated 2-byte should be 0xFFFD(1)");
  asrt_msg(chars_is_eof(&iter), "Should be EOF after truncated byte");

  // 3. 截断的 4-byte 序列 (后跟一个有效字符)
  strslice_t s3 = SLICE_LITERAL("\xF0\x9F\x9A"
                                "a"); // "a"
  chars_init(&iter, s3);

  ok = chars_consume(&iter, &c); // F0
  asrt_msg(ok && c.codepoint == 0xFFFD && c.width == 1, "Truncated 4-byte (1)");
  ok = chars_consume(&iter, &c); // 9F
  asrt_msg(ok && c.codepoint == 0xFFFD && c.width == 1, "Truncated 4-byte (2)");
  ok = chars_consume(&iter, &c); // 9A
  asrt_msg(ok && c.codepoint == 0xFFFD && c.width == 1, "Truncated 4-byte (3)");

  ok = chars_consume(&iter, &c); // a
  asrt_msg(ok && c.codepoint == 'a' && c.width == 1,
           "Should consume 'a' after invalid seq");
  asrt_msg(chars_is_eof(&iter), "Should be EOF at the end");

  // 4. Overlong 编码 (C0 80 是 NUL 的非法 2-byte 形式)
  strslice_t s4 = SLICE_LITERAL("\xC0\x80");
  chars_init(&iter, s4);
  ok = chars_consume(&iter, &c); // C0
  asrt_msg(ok && c.codepoint == 0xFFFD && c.width == 1, "Overlong C0 failed");
  ok = chars_consume(&iter, &c); // 80
  asrt_msg(ok && c.codepoint == 0xFFFD && c.width == 1, "Overlong 80 failed");
  asrt_msg(chars_is_eof(&iter), "Should be EOF after overlong");

  // 5. UTF-16 代理对 (Surrogate) (在 UTF-8 中非法)
  strslice_t s5 = SLICE_LITERAL("\xED\xA0\x80"); // U+D800
  chars_init(&iter, s5);
  ok = chars_consume(&iter, &c); // ED
  asrt_msg(ok && c.codepoint == 0xFFFD && c.width == 1, "Surrogate ED failed");
  ok = chars_consume(&iter, &c); // A0
  asrt_msg(ok && c.codepoint == 0xFFFD && c.width == 1, "Surrogate A0 failed");
  ok = chars_consume(&iter, &c); // 80
  asrt_msg(ok && c.codepoint == 0xFFFD && c.width == 1, "Surrogate 80 failed");
  asrt_msg(chars_is_eof(&iter), "Should be EOF after surrogate");
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

  printf("=== [fluf] Running tests for <std/string/chars> ===\n");

  test_init_and_eof();
  test_ascii_string();
  test_multibyte_string();
  test_invalid_utf8();

  printf("==================================================\n");
  printf("✅ All <std/string/chars> tests passed!\n");
  printf("==================================================\n");

  return 0;
}