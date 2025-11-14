#pragma once

#include <stddef.h>

/**
 * @brief (结构体) 表示一个半开半闭区间 [start, end)
 */
typedef struct span {
  size_t start;
  size_t end;
} span_t;

/**
 * @brief (构造函数) 创建一个 span_t 结构体
 *
 * @note 如果 start > end, 结果会是一个空范围 (start ==
 * end)。
 */
static inline span_t range(size_t start, size_t end) {
  return (span_t){.start = start, .end = (start > end ? start : end)};
}

/**
 * @brief (构造函数) 从 [start, start + len) 范围创建一个 span
 * (这个在词法分析器 (Lexer) 中非常有用)
 */
static inline span_t span_from_len(size_t start, size_t len) {
  return (span_t){.start = start, .end = start + len};
}

/**
 * @brief (辅助函数) 合并两个 span
 *
 * (这个在解析器 (Parser) 中至关重要，
 * 例如 `(a + b)` 的 span = `span_merge(a.span, b.span)`)
 */
static inline span_t span_merge(span_t a, span_t b) {

  size_t start = (a.start < b.start) ? a.start : b.start;
  size_t end = (a.end > b.end) ? a.end : b.end;
  return (span_t){.start = start, .end = end};
}

/**
 * @brief (辅助函数) 获取 span 的长度
 */
static inline size_t span_len(span_t span) { return span.end - span.start; }

/**
 * @brief (宏) 遍历一个字面量范围 [start, end)
 *
 * @example
 * for_range(i, 0, 10) { // i 将从 0 到 9
 *   dbg("i = %zu", i);
 *  }
 */
#define for_range(var, start, end)                                             \
  for (size_t var = (start); var < (end); var++)

/**
 * @brief (宏) 遍历一个 span_t 结构体
 *
 * @example
 * span_t r = range(5, 10);
 * for_range_in(i, r) { // i 将从 5 到 9
 *  dbg("i = %zu", i);
 * }
 */
#define for_range_in(var, range_obj)                                           \
  for (size_t var = (range_obj).start; var < (range_obj).end; var++)
