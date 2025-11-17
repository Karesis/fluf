/*
 *    Copyright 2025 Karesis
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <core/mem/allocer.h>
#include <core/mem/layout.h>
#include <core/msg/asrt.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/**
 * @brief 字符串切片 (String Slice / View)
 *
 * 一个非拥有 (non-owning) 的 "胖指针"，代表一个*不一定*
 * 以 '\0' 结尾的字符串视图。
 *
 * 它的核心优势是 `len` 是 O(1) 的，并且它可以
 * "切片" (slice) 现有的内存（如源文件），而无需复制。
 */
typedef struct str_slice {
  const char *ptr;
  size_t len;
} strslice_t;

/**
 * @brief (辅助宏) 从 C 字符串字面量创建切片
 *
 * 示例: strslice_t s = SLICE_LITERAL("hello");
 * (s.ptr = "hello", s.len = 5)
 * * sizeof("hello") == 6 (包含 \0)
 */
#define SLICE_LITERAL(s) ((strslice_t){.ptr = (s), .len = sizeof(s) - 1})

/**
 * @brief (辅助函数) 从 C 字符串 (const char*) 创建切片
 * * 这是一个 O(n) 操作，因为它调用了 strlen。
 */
static inline strslice_t slice_from_cstr(const char *cstr) {
  asrt_msg(cstr != NULL, "Cannot create slice from NULL c-string");
  return (strslice_t){.ptr = cstr, .len = strlen(cstr)};
}

/**
 * @brief (辅助函数) 比较两个切片是否相等 (O(n))
 */
static inline bool slice_equals(strslice_t a, strslice_t b) {
  if (a.len != b.len) {
    return false;
  }
  // 必须使用 memcmp, 因为 strncmp 会在 \0 处停止
  return memcmp(a.ptr, b.ptr, a.len) == 0;
}

/**
 * @brief (辅助函数) 比较切片和一个 C 字符串是否相等 (O(n))
 */
static inline bool slice_equals_cstr(strslice_t a, const char *b_cstr) {
  // strncmp 在这里是安全的，因为 b_cstr 保证以 \0 结尾
  size_t b_len = strlen(b_cstr);
  if (a.len != b_len) {
    return false;
  }
  return strncmp(a.ptr, b_cstr, a.len) == 0;
}

/**
 * @brief (辅助函数) 检查切片是否以指定前缀 (slice) 开头
 */
static inline bool slice_starts_with(strslice_t s, strslice_t prefix) {
  if (s.len < prefix.len) {
    return false;
  }
  return memcmp(s.ptr, prefix.ptr, prefix.len) == 0;
}

/**
 * @brief (辅助函数) 检查切片是否以指定前缀 (C-string) 开头
 */
static inline bool slice_starts_with_cstr(strslice_t s,
                                          const char *prefix_cstr) {
  size_t prefix_len = strlen(prefix_cstr); // <-- 修复
  if (s.len < prefix_len) {
    return false;
  }
  return memcmp(s.ptr, prefix_cstr, prefix_len) == 0;
}

/**
 * @brief (辅助函数) 检查切片是否以指定后缀 (slice) 结尾
 */
static inline bool slice_ends_with(strslice_t s, strslice_t suffix) {
  if (s.len < suffix.len) {
    return false;
  }
  return memcmp(s.ptr + s.len - suffix.len, suffix.ptr, suffix.len) == 0;
}

/**
 * @brief (辅助函数) 检查切片是否以指定后缀 (C-string) 结尾
 */
static inline bool slice_ends_with_cstr(strslice_t s, const char *suffix_cstr) {
  size_t suffix_len = strlen(suffix_cstr); // <-- 新增
  if (s.len < suffix_len) {
    return false;
  }
  return memcmp(s.ptr + s.len - suffix_len, suffix_cstr, suffix_len) == 0;
}

/** (内部辅助) 检查一个 char 是否是 `cnote` 需要的空白符 */
static inline bool _slice_is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/**
 * @brief 修剪切片左侧（开头）的空白符
 */
static inline strslice_t slice_trim_left(strslice_t s) {
  size_t start = 0;
  while (start < s.len && _slice_is_whitespace(s.ptr[start])) {
    start++;
  }
  return (strslice_t){.ptr = s.ptr + start, .len = s.len - start};
}

/**
 * @brief 修剪切片右侧（末尾）的空白符
 */
static inline strslice_t slice_trim_right(strslice_t s) {
  size_t end = s.len;
  while (end > 0 && _slice_is_whitespace(s.ptr[end - 1])) {
    end--;
  }
  return (strslice_t){.ptr = s.ptr, .len = end};
}

/**
 * @brief 修剪切片两端的空白符
 */
static inline strslice_t slice_trim(strslice_t s) {
  return slice_trim_right(slice_trim_left(s));
}

/**
 * @brief (迭代器) 从切片中分离下一个子切片
 *
 * 这是一个 `fluf` 风格的 `strtok`。它会修改 `s_ptr` (输入切片)。
 *
 * @param s_ptr [in/out] 指向要进行 `split` 的切片的指针。
 * @param delim 用于分割的分隔符。
 * @param out_result [out] 存储找到的下一个子切片 (不含分隔符)。
 * @return true (如果找到了一个切片) 或 false (如果 `s_ptr` 已经是空的)。
 *
 * @example
 * strslice_t s = SLICE_LITERAL("a,b,c");
 * strslice_t token;
 * while (slice_split_next(&s, ',', &token)) {
 * // 第一次: token = "a", s = "b,c"
 * // 第二次: token = "b", s = "c"
 * // 第三次: token = "c", s = ""
 * }
 */
static inline bool slice_split_next(strslice_t *s_ptr, char delim,
                                    strslice_t *out_result) {
  if (s_ptr->len == 0 && s_ptr->ptr == NULL) {
    // (ptr == NULL 是一个哨兵，表示迭代已完成)
    return false;
  }

  // 1. 查找分隔符
  void *delim_ptr = memchr(s_ptr->ptr, delim, s_ptr->len);

  if (delim_ptr != NULL) {
    // 2. 找到了分隔符
    size_t pos = (const char *)delim_ptr - s_ptr->ptr;

    // a. 设置结果
    *out_result = (strslice_t){.ptr = s_ptr->ptr, .len = pos};

    // b. 修改输入切片 (跳过分隔符)
    s_ptr->ptr += (pos + 1);
    s_ptr->len -= (pos + 1);

    return true;
  } else {
    // 3. 未找到分隔符 (这是最后一个切片)

    // a. 设置结果 (为剩余的整个切片)
    *out_result = *s_ptr;

    // b. 修改输入切片 (设置为空哨兵)
    *s_ptr = (strslice_t){.ptr = NULL, .len = 0};

    return true;
  }
}

/**
 * @brief (核心) 复制一个 `strslice_t` 到分配器中。
 *
 * (fluf 版本的 strndup)
 * 这将创建一个*新的*、以 '\0' 结尾的 C 字符串。
 *
 * @param slice 要复制的切片 (不要求 \0 结尾)。
 * @param alc 用于分配新字符串的分配器。
 * @return 一个指向新副本的 `char*`，或 OOM 时返回 NULL。
 */
static inline char *slice_dup(strslice_t slice, allocer_t *alc) {
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  // (len + 1) 用于 \0
  layout_t layout = layout_of_array(char, slice.len + 1);

  char *new_str = (char *)allocer_alloc(alc, layout);
  if (new_str == NULL) {
    return NULL; // OOM
  }

  memcpy(new_str, slice.ptr, slice.len);
  new_str[slice.len] = '\0'; // 确保 \0 结尾

  return new_str;
}