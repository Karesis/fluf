#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @file
 * @brief Unicode 属性检查工具。
 * (模仿 rustc_lexer 和 unicode_xid 的功能)
 */

/**
 * @brief (rustc_lexer::is_whitespace)
 * 检查 c 是否为 Rust 定义的空白字符。
 * (U+000A, U+000B, U+000C, U+000D, U+0085, U+2028, U+2029)
 * (U+200E, U+200F)
 * (U+0009, U+0020)
 */
bool unicode_is_whitespace(uint32_t c);

/**
 * @brief (rustc_lexer::is_id_start)
 * 检查 c 是否为合法的标识符起始字符 (XID_Start 或 '_')。
 *
 * @note
 * 这是一个 "fluf-practical" 实现。它 100% 正确处理 ASCII，
 * 并为非 ASCII 范围提供一个（目前）宽容的存根 (stub)。
 * TODO: 填充完整的 Unicode XID_Start 表。
 */
bool unicode_is_id_start(uint32_t c);

/**
 * @brief (rustc_lexer::is_id_continue)
 * 检查 c 是否为合法的标识符非起始字符 (XID_Continue)。
 *
 * @note
 * 这是一个 "fluf-practical" 实现。它 100% 正确处理 ASCII，
 * 并为非 ASCII 范围提供一个（目前）宽容的存根 (stub)。
 * TODO: 填充完整的 Unicode XID_Continue 表。
 */
bool unicode_is_id_continue(uint32_t c);

/**
 * @brief 检查 c 是否为十进制数字 ('0'..'9')
 */
bool unicode_is_decimal_digit(uint32_t c);

/**
 * @brief 检查 c 是否为十六进制数字 ('0'..'9', 'a'..'f', 'A'..'F')
 */
bool unicode_is_hex_digit(uint32_t c);