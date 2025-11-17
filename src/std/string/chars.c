#include <core/msg/asrt.h> // 引入 fluf 的断言宏
#include <std/string/chars.h>

/**
 * @brief 内部辅助函数：在给定的字节偏移量处解码一个 UTF-8 字符。
 *
 * 这是这个模块的核心。它不修改任何状态，只是“查看”。
 *
 * @param slice     源字符串切片
 * @param offset    要解码的字节偏移量
 * @param out_char  用于接收解码结果的指针
 * @return true 如果成功解码了一个字符 (包括 EOF)，false 如果已达 EOF。
 */
static bool _chars_decode_at(strslice_t slice, size_t offset,
                             utf8_t *out_char) {
  // 1. EOF 检查
  if (offset >= slice.len) {
    return false; // 到达末尾
  }

  const uint8_t *data = (const uint8_t *)slice.ptr;
  uint8_t b1 = data[offset];

  // 2. 快速路径：1 字节 (ASCII)
  // 范围: U+0000 - U+007F
  // 模式: 0xxxxxxx
  if (b1 < 0x80) {
    out_char->codepoint = (uint32_t)b1;
    out_char->width = 1;
    return true;
  }

  // --- 错误和多字节处理 ---
  // 检查 b1 是不是一个合法的多字节序列的 *开头*

  // 3. 处理 2 字节序列
  // 范围: U+0080 - U+07FF
  // 模式: 110xxxxx 10xxxxxx
  if ((b1 & 0xE0) == 0xC0) {
    // C0 和 C1 是非法的 (用于 overlong 编码)
    if (b1 < 0xC2) {
      goto error_invalid_byte;
    }

    // 检查序列是否完整
    if (offset + 1 >= slice.len) {
      goto error_invalid_byte; // 序列被截断
    }

    uint8_t b2 = data[offset + 1];
    if ((b2 & 0xC0) != 0x80) {
      goto error_invalid_byte; // b2 不是一个 continuation byte
    }

    out_char->codepoint = ((uint32_t)(b1 & 0x1F) << 6) | (uint32_t)(b2 & 0x3F);
    out_char->width = 2;
    return true;
  }

  // 4. 处理 3 字节序列
  // 范围: U+0800 - U+FFFF
  // 模式: 1110xxxx 10xxxxxx 10xxxxxx
  if ((b1 & 0xF0) == 0xE0) {
    if (offset + 2 >= slice.len) {
      goto error_invalid_byte; // 序列被截断
    }

    uint8_t b2 = data[offset + 1];
    uint8_t b3 = data[offset + 2];
    if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
      goto error_invalid_byte; // 非 continuation bytes
    }

    // 检查 overlong 编码 (例如 E0 80 80 是非法的)
    if (b1 == 0xE0 && b2 < 0xA0) {
      goto error_invalid_byte;
    }

    // 检查 UTF-16 代理对 (Surrogates)，U+D800 - U+DFFF
    // 它们在 UTF-8 中是非法的
    if (b1 == 0xED && b2 >= 0xA0) {
      goto error_invalid_byte;
    }

    out_char->codepoint = ((uint32_t)(b1 & 0x0F) << 12) |
                          ((uint32_t)(b2 & 0x3F) << 6) | (uint32_t)(b3 & 0x3F);
    out_char->width = 3;
    return true;
  }

  // 5. 处理 4 字节序列
  // 范围: U+10000 - U+10FFFF
  // 模式: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
  if ((b1 & 0xF8) == 0xF0) {
    if (offset + 3 >= slice.len) {
      goto error_invalid_byte; // 序列被截断
    }

    uint8_t b2 = data[offset + 1];
    uint8_t b3 = data[offset + 2];
    uint8_t b4 = data[offset + 3];
    if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80 || (b4 & 0xC0) != 0x80) {
      goto error_invalid_byte; // 非 continuation bytes
    }

    // 检查 overlong 编码
    if (b1 == 0xF0 && b2 < 0x90) {
      goto error_invalid_byte;
    }

    // 检查是否超出 Unicode 范围 (U+10FFFF)
    // F4 90 xx xx 及以上都是非法的
    if (b1 > 0xF4 || (b1 == 0xF4 && b2 >= 0x90)) {
      goto error_invalid_byte;
    }

    out_char->codepoint = ((uint32_t)(b1 & 0x07) << 18) |
                          ((uint32_t)(b2 & 0x3F) << 12) |
                          ((uint32_t)(b3 & 0x3F) << 6) | (uint32_t)(b4 & 0x3F);
    out_char->width = 4;
    return true;
  }

  // 6. 错误处理：非法的起始字节
  // (例如 10xxxxxx 或 11111xxx)
error_invalid_byte:
  // 返回 Unicode 替换字符 ()
  // 这是最健壮的做法，允许 Lexer 继续报告更多错误
  out_char->codepoint = 0xFFFD;
  out_char->width = 1; // 安全地消耗掉这个坏字节
  return true;
}

// --- 公共 API 实现 ---

void chars_init(chars_t *iter, strslice_t slice) {
  // asrt_msg 是 fluf 的风格，用于检查合约
  asrt_msg(iter != NULL, "chars_init: 'iter' cannot be NULL");
  iter->slice = slice;
  iter->offset = 0;
}

bool chars_peek(chars_t *iter, utf8_t *out_char) {
  asrt_msg(iter != NULL, "chars_peek: 'iter' cannot be NULL");
  asrt_msg(out_char != NULL, "chars_peek: 'out_char' cannot be NULL");

  return _chars_decode_at(iter->slice, iter->offset, out_char);
}

bool chars_consume(chars_t *iter, utf8_t *out_char) {
  asrt_msg(iter != NULL, "chars_consume: 'iter' cannot be NULL");

  utf8_t c;
  if (!_chars_decode_at(iter->slice, iter->offset, &c)) {
    // 到达 EOF
    return false;
  }

  // 成功解码，推进光标
  iter->offset += c.width;

  if (out_char != NULL) {
    *out_char = c;
  }
  return true;
}

bool chars_advance(chars_t *iter) {
  asrt_msg(iter != NULL, "chars_advance: 'iter' cannot be NULL");

  utf8_t c; // 临时的
  if (!_chars_decode_at(iter->slice, iter->offset, &c)) {
    return false; // EOF
  }

  // 成功，推进光标
  iter->offset += c.width;
  return true;
}

size_t chars_offset(const chars_t *iter) {
  asrt_msg(iter != NULL, "chars_offset: 'iter' cannot be NULL");
  return iter->offset;
}

bool chars_is_eof(const chars_t *iter) {
  asrt_msg(iter != NULL, "chars_is_eof: 'iter' cannot be NULL");
  return iter->offset >= iter->slice.len;
}