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
#include <std/string/str_slice.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 动态字符串构建器
 *
 * 一个有状态的、可变的字符串。它拥有自己的内存，
 * 并保证*始终*以 '\0' 结尾。
 *
 * (遵循 bump_t/interner_t/vec_t 模式, 在栈上初始化)
 */
typedef struct string {
  allocer_t *alc;
  char *data;
  size_t count;
  size_t capacity;
} string_t;

/**
 * @brief 初始化一个 string (例如在栈上)。
 *
 * @param s 指向要初始化的 string 实例的指针。
 * @param alc 用于所有内部存储的分配器 (vtable 句柄)。
 * @param initial_capacity *不*包含 \0 的初始容量。
 * @return true (成功) 或 false (OOM)。
 */
bool string_init(string_t *s, allocer_t *alc, size_t initial_capacity);

/**
 * @brief 销毁 string 的内部数据 (data 数组)。
 */
void string_destroy(string_t *s);

/**
 * @brief 在分配器上创建一个新的 string。
 *
 * 这会从 `alc` 为 `string_t` 结构体*本身*分配内存。
 * `string_t` 的生命周期现在与 `alc` (例如 Arena) 绑定。
 *
 * @param alc 分配器。
 * @param initial_capacity 初始容量。
 * @return 指向 Arena 上的 `string_t` 的指针，或 OOM 时返回 NULL。
 */
static inline string_t *string_new(allocer_t *alc, size_t initial_capacity) {
  // 1. 在 Arena 上为 string_t 结构体分配内存
  string_t *s = (string_t *)allocer_alloc(alc, layout_of(string_t));
  if (s == NULL) {
    return NULL; // OOM
  }

  // 2. 调用 _init 来初始化它
  if (!string_init(s, alc, initial_capacity)) {
    return NULL; // OOM
  }

  return s;
}

/**
 * @brief 将一个字符 (char) 追加到字符串末尾。
 *
 * @param s string 实例。
 * @param c 要追加的字符。
 * @return true (成功) 或 false (OOM / 扩容失败)。
 */
bool string_push(string_t *s, char c);

/**
 * @brief 将一个 C-string (const char*) 追加到字符串末尾。
 *
 * @param s string 实例。
 * @param cstr 要追加的 C 字符串 (以 \0 结尾)。
 * @return true (成功) 或 false (OOM / 扩容失败)。
 */
bool string_append_cstr(string_t *s, const char *cstr);

/**
 * @brief 将一个字符串切片 (str_slice_t) 追加到字符串末尾。
 *
 * @param s string 实例。
 * @param slice 要追加的切片 (不要求 \0 结尾)。
 * @return true (成功) 或 false (OOM / 扩容失败)。
 */
bool string_append_slice(string_t *s, str_slice_t slice);

/**
 * @brief 将 printf 风格的格式化字符串追加到末尾。
 *
 * @param s string 实例。
 * @param fmt printf 风格的格式化字符串。
 * @param ... 格式化参数。
 * @return true (成功) 或 false (OOM / 扩容失败)。
 */
bool string_append_fmt(string_t *s, const char *fmt, ...);

/**
 * @brief 清空字符串 (count=0) 但保留容量。
 */
void string_clear(string_t *s);

/**
 * @brief (O(1)) 将 string 作为 C-string (const char*) 返回。
 * * 保证以 \0 结尾。
 *
 * @warning
 * **生命周期陷阱 (Lifetime Trap)!**
 * 返回的指针*直接指向* string_t 的内部缓冲区。
 * *任何*后续的 `string_push`、`string_append_*`、`string_clear`
 * 或 `string_destroy` 调用都可能导致此指针**立即失效** *
 * (变为悬垂指针)，因为它可能触发 `realloc`。
 *
 * 如果你需要在循环、递归或跨越修改操作时持有此字符串，
 * **必须**使用 `cstr_dup(string_as_cstr(s), alc)` 来创建一个安全副本。
 */
const char *string_as_cstr(const string_t *s);

/**
 * @brief (O(1)) 将 string 作为 str_slice_t 返回。
 */
str_slice_t string_as_slice(const string_t *s);

/**
 * @brief (O(1)) 获取字符串长度 (strlen)。
 */
size_t string_count(const string_t *s);