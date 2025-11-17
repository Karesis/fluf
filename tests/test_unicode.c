#include <stdio.h>

// --- Fluf 依赖 ---
#include <core/msg/asrt.h>

// --- 我们要测试的模块 ---
#include <std/unicode.h>

/**
 * @brief 测试 1：Whitespace
 */
static void test_whitespace(void) {
  printf("--- Test: test_whitespace ---\n");

  // 水平
  asrt_msg(unicode_is_whitespace('\t'), "Tab failed");
  asrt_msg(unicode_is_whitespace(' '), "Space failed");

  // 垂直
  asrt_msg(unicode_is_whitespace('\n'), "LF failed");
  asrt_msg(unicode_is_whitespace('\r'), "CR failed");
  asrt_msg(unicode_is_whitespace(0x000B), "VT failed");
  asrt_msg(unicode_is_whitespace(0x000C), "FF failed");
  asrt_msg(unicode_is_whitespace(0x0085), "NEL failed");
  asrt_msg(unicode_is_whitespace(0x2028), "LS failed");
  asrt_msg(unicode_is_whitespace(0x2029), "PS failed");

  // 忽略
  asrt_msg(unicode_is_whitespace(0x200E), "LRM failed");
  asrt_msg(unicode_is_whitespace(0x200F), "RLM failed");

  // 阴性测试 (Negative)
  asrt_msg(!unicode_is_whitespace('a'), "Negative 'a' failed");
  asrt_msg(!unicode_is_whitespace('_'), "Negative '_' failed");
  asrt_msg(!unicode_is_whitespace('0'), "Negative '0' failed");
  asrt_msg(!unicode_is_whitespace(0), "Negative NUL failed");
}

/**
 * @brief 测试 2：Digits
 */
static void test_digits(void) {
  printf("--- Test: test_digits ---\n");

  // 十进制
  asrt_msg(unicode_is_decimal_digit('0'), "Digit '0' failed");
  asrt_msg(unicode_is_decimal_digit('9'), "Digit '9' failed");
  asrt_msg(!unicode_is_decimal_digit('a'), "Negative 'a' (decimal) failed");
  asrt_msg(!unicode_is_decimal_digit('G'), "Negative 'G' (decimal) failed");

  // 十六进制
  asrt_msg(unicode_is_hex_digit('0'), "Hex '0' failed");
  asrt_msg(unicode_is_hex_digit('9'), "Hex '9' failed");
  asrt_msg(unicode_is_hex_digit('a'), "Hex 'a' failed");
  asrt_msg(unicode_is_hex_digit('f'), "Hex 'f' failed");
  asrt_msg(unicode_is_hex_digit('A'), "Hex 'A' failed");
  asrt_msg(unicode_is_hex_digit('F'), "Hex 'F' failed");
  asrt_msg(!unicode_is_hex_digit('g'), "Negative 'g' (hex) failed");
  asrt_msg(!unicode_is_hex_digit('Z'), "Negative 'Z' (hex) failed");
  asrt_msg(!unicode_is_hex_digit('_'), "Negative '_' (hex) failed");
}

/**
 * @brief 测试 3：Identifier (基于当前的存根实现)
 */
static void test_id_stubs(void) {
  printf("--- Test: test_id_stubs ---\n");

  // --- ID Start ---
  asrt_msg(unicode_is_id_start('_'), "ID_Start '_' failed");
  asrt_msg(unicode_is_id_start('a'), "ID_Start 'a' failed");
  asrt_msg(unicode_is_id_start('Z'), "ID_Start 'Z' failed");
  asrt_msg(!unicode_is_id_start('0'), "ID_Start '0' (negative) failed");
  asrt_msg(!unicode_is_id_start('9'), "ID_Start '9' (negative) failed");
  asrt_msg(!unicode_is_id_start('$'), "ID_Start '$' (negative) failed");
  asrt_msg(!unicode_is_id_start(' '), "ID_Start ' ' (negative) failed");

  // --- ID Continue ---
  asrt_msg(unicode_is_id_continue('_'), "ID_Continue '_' failed");
  asrt_msg(unicode_is_id_continue('a'), "ID_Continue 'a' failed");
  asrt_msg(unicode_is_id_continue('Z'), "ID_Continue 'Z' failed");
  asrt_msg(unicode_is_id_continue('0'), "ID_Continue '0' failed");
  asrt_msg(unicode_is_id_continue('9'), "ID_Continue '9' failed");
  asrt_msg(!unicode_is_id_continue('$'), "ID_Continue '$' (negative) failed");
  asrt_msg(!unicode_is_id_continue(' '), "ID_Continue ' ' (negative) failed");

  // --- 非 ASCII (存根测试) ---
  // (我们的存根接受非空白的非 ASCII 字符)
  asrt_msg(unicode_is_id_start(0xE9), "ID_Start 'é' (stub) failed");    // é
  asrt_msg(unicode_is_id_start(0x4E2D), "ID_Start '中' (stub) failed"); // 中
  asrt_msg(unicode_is_id_continue(0xE9), "ID_Continue 'é' (stub) failed");
  asrt_msg(unicode_is_id_continue(0x4E2D), "ID_Continue '中' (stub) failed");

  // (我们的存根会*拒绝*非 ASCII 的空白)
  asrt_msg(!unicode_is_id_start(0x2028),
           "ID_Start 'LS' (negative stub) failed");
  asrt_msg(!unicode_is_id_continue(0x2028),
           "ID_Continue 'LS' (negative stub) failed");
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

  printf("=== [fluf] Running tests for <std/unicode/unicode> ===\n");

  test_whitespace();
  test_digits();
  test_id_stubs();

  printf("=====================================================\n");
  printf("✅ All <std/unicode/unicode> tests passed!\n");
  printf("=====================================================\n");

  return 0;
}