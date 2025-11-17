#pragma once

#include <std/string/strslice.h>
#include <stdint.h>

typedef struct utf8 {
  uint32_t codepoint;
  uint8_t width;
} utf8_t;

typedef struct chars {
  strslice_t slice;
  size_t offset;
} chars_t;

/**
 * @brief 初始化一个字符迭代器。
 * @param iter  要被初始化的迭代器指针 (在栈上)。
 * @param slice 包含 UTF-8 文本的源字符串切片。
 */
void chars_init(chars_t *iter, strslice_t slice);

/**
 * @brief "偷看" (Peek) 下一个字符，但不推进迭代器。
 *
 * @param iter     迭代器。
 * @param out_char 用于接收结果的指针。
 * @return true 如果成功 peek (out_char 被填充)，false 如果已达 EOF。
 */
bool chars_peek(chars_t *iter, utf8_t *out_char);

/**
 * @brief "消耗" (Consume) 下一个字符，并推进迭代器。
 *
 * @param iter     迭代器。
 * @param out_char 用于接收结果的指针 (可选, 可传 NULL)。
 * @return true 如果成功 consume，false 如果已达 EOF。
 */
bool chars_consume(chars_t *iter, utf8_t *out_char);

/**
 * @brief "消耗" (Consume) 下一个字符 (如果不关心结果，这是更快的版本)。
 * @return true 如果成功 consume，false 如果已达 EOF。
 */
bool chars_advance(chars_t *iter);

// --- `CharIndices` 的功能 ---

/**
 * @brief 获取迭代器当前所在的字节偏移量。
 */
size_t chars_offset(const chars_t *iter);

/**
 * @brief 检查迭代器是否已到达末尾。
 */
bool chars_is_eof(const chars_t *iter);
