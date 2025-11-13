#pragma once

#include <stddef.h> // for size_t
#include <string.h> // for strlen, memcmp, strncmp
#include <core/msg/asrt.h>
#include <stdbool.h>

/**
 * @brief 字符串切片 (String Slice / View)
 *
 * 一个非拥有 (non-owning) 的 "胖指针"，代表一个*不一定*
 * 以 '\0' 结尾的字符串视图。
 *
 * 它的核心优势是 `len` 是 O(1) 的，并且它可以
 * "切片" (slice) 现有的内存（如源文件），而无需复制。
 */
typedef struct
{
  const char *ptr;
  size_t len;
} str_slice_t;

/**
 * @brief (辅助宏) 从 C 字符串字面量创建切片
 *
 * 示例: str_slice_t s = SLICE_LITERAL("hello");
 * (s.ptr = "hello", s.len = 5)
 * * sizeof("hello") == 6 (包含 \0)
 */
#define SLICE_LITERAL(s) ((str_slice_t){.ptr = (s), .len = sizeof(s) - 1})

/**
 * @brief (辅助函数) 从 C 字符串 (const char*) 创建切片
 * * 这是一个 O(n) 操作，因为它调用了 strlen。
 */
static inline str_slice_t
slice_from_cstr(const char *cstr)
{
  asrt_msg(cstr != NULL, "Cannot create slice from NULL c-string");
  return (str_slice_t){.ptr = cstr, .len = strlen(cstr)};
}

/**
 * @brief (辅助函数) 比较两个切片是否相等 (O(n))
 */
static inline bool
slice_equals(str_slice_t a, str_slice_t b)
{
  if (a.len != b.len)
  {
    return false;
  }
  // 我们必须使用 memcmp, 因为 strncmp 会在 \0 处停止
  // (我们的切片中可能包含 \0)
  return memcmp(a.ptr, b.ptr, a.len) == 0;
}

/**
 * @brief (辅助函数) 比较切片和一个 C 字符串是否相等 (O(n))
 */
static inline bool
slice_equals_cstr(str_slice_t a, const char *b_cstr)
{
  // strncmp 在这里是安全的，因为 b_cstr 保证以 \0 结尾
  size_t b_len = strlen(b_cstr);
  if (a.len != b_len)
  {
    return false;
  }
  return strncmp(a.ptr, b_cstr, a.len) == 0;
}