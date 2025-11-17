#include <std/unicode.h>

// (来自 rustc_lexer::is_whitespace 的完整、稳定实现)
bool unicode_is_whitespace(uint32_t c) {
  switch (c) {
  // 水平空白 (Horizontal)
  case 0x0009: // \t (Tab)
  case 0x0020: // ' ' (Space)
  // 垂直空白 (Vertical)
  case 0x000A: // \n (Line Feed)
  case 0x000B: // (Vertical Tab)
  case 0x000C: // (Form Feed)
  case 0x000D: // \r (Carriage Return)
  case 0x0085: // (Next Line)
  case 0x2028: // (Line Separator)
  case 0x2029: // (Paragraph Separator)
  // 忽略字符 (Ignorable)
  case 0x200E: // (Left-to-Right Mark)
  case 0x200F: // (Right-to-Left Mark)
    return true;
  default:
    return false;
  }
}

bool unicode_is_id_start(uint32_t c) {
  // 1. ASCII 快速路径 (100% 准确)
  if (c <= 0x7F) {
    if (c == '_') {
      return true;
    }
    // a-z
    if (c >= 'a' && c <= 'z') {
      return true;
    }
    // A-Z
    if (c >= 'A' && c <= 'Z') {
      return true;
    }
    return false;
  }

  // 2. 非 ASCII 存根 (Stub)
  // TODO: 在这里实现一个二分查找，
  // 查找从 unicode_xid 库导出的 XID_Start 范围表。

  // 目前，我们“宽容地”接受所有非 ASCII、非空白字符作为起始
  // 这对于 Lexer 的健壮性来说比“拒绝”要好
  return !unicode_is_whitespace(c);
}

bool unicode_is_id_continue(uint32_t c) {
  // 1. ASCII 快速路径 (100% 准确)
  if (c <= 0x7F) {
    if (c == '_') {
      return true;
    }
    // a-z
    if (c >= 'a' && c <= 'z') {
      return true;
    }
    // A-Z
    if (c >= 'A' && c <= 'Z') {
      return true;
    }
    // 0-9
    if (c >= '0' && c <= '9') {
      return true;
    }
    return false;
  }

  // 2. 非 ASCII 存根 (Stub)
  // TODO: 在这里实现一个二分查找，
  // 查找从 unicode_xid 库导出的 XID_Continue 范围表。

  return !unicode_is_whitespace(c);
}

bool unicode_is_decimal_digit(uint32_t c) { return c >= '0' && c <= '9'; }

bool unicode_is_hex_digit(uint32_t c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}