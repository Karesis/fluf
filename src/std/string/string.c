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

#include <core/mem/allocer.h>
#include <core/mem/layout.h>
#include <core/msg/asrt.h>
#include <std/string/string.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define STRING_DEFAULT_CAPACITY 8

/**
 * @brief 内部扩容函数
 * * 确保有足够的空间容纳 `needed` 个*额外*的字节，
 * * 并始终保留 `\0` 的空间。
 * @return true (成功) 或 false (OOM)
 */
static bool string_grow(string_t *s, size_t needed) {
  size_t new_count = s->count + needed;
  if (new_count <= s->capacity) {
    return true;
  }

  size_t new_capacity = s->capacity * 2;
  if (new_capacity == 0)
    new_capacity = STRING_DEFAULT_CAPACITY;
  while (new_capacity < new_count) {
    new_capacity *= 2;
  }

  layout_t old_layout = layout_of_array(char, s->capacity + 1);
  layout_t new_layout = layout_of_array(char, new_capacity + 1);

  char *new_data;
  if (s->capacity == 0) {
    new_data = allocer_alloc(s->alc, new_layout);
  } else {
    new_data = allocer_realloc(s->alc, s->data, old_layout, new_layout);
  }

  if (!new_data) {
    return false;
  }

  s->data = new_data;
  s->capacity = new_capacity;
  return true;
}

bool string_init(string_t *s, allocer_t *alc, size_t initial_capacity) {
  asrt_msg(s != NULL, "String pointer cannot be NULL");
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  size_t capacity =
      (initial_capacity > 0) ? initial_capacity : STRING_DEFAULT_CAPACITY;

  layout_t layout = layout_of_array(char, capacity + 1);
  char *data = allocer_alloc(alc, layout);

  if (!data) {
    memset(s, 0, sizeof(string_t));
    return false;
  }

  s->alc = alc;
  s->data = data;
  s->capacity = capacity;
  s->count = 0;

  s->data[0] = '\0';

  return true;
}

void string_destroy(string_t *s) {
  if (!s)
    return;

  if (s->data) {

    layout_t layout = layout_of_array(char, s->capacity + 1);
    allocer_free(s->alc, s->data, layout);
  }

  memset(s, 0, sizeof(string_t));
}

bool string_push(string_t *s, char c) {

  if (!string_grow(s, 1)) {
    return false;
  }

  s->data[s->count] = c;
  s->count++;

  s->data[s->count] = '\0';

  return true;
}

bool string_append_cstr(string_t *s, const char *cstr) {
  size_t len = strlen(cstr);
  if (len == 0)
    return true;

  if (!string_grow(s, len)) {
    return false;
  }

  memcpy(s->data + s->count, cstr, len);
  s->count += len;

  s->data[s->count] = '\0';

  return true;
}

bool string_append_slice(string_t *s, str_slice_t slice) {
  if (slice.len == 0)
    return true;

  if (!string_grow(s, slice.len)) {
    return false;
  }

  memcpy(s->data + s->count, slice.ptr, slice.len);
  s->count += slice.len;

  s->data[s->count] = '\0';

  return true;
}

/**
 * @brief string_append_fmt 的 va_list 版本 (内部辅助)
 *
 */
static bool string_append_vfmt(string_t *s, const char *fmt, va_list args) {
  // --- 1. 第一次调用 vsnprintf (两阶段技巧) ---
  // C 语言标准：vsnprintf(NULL, 0, ...) 会返回*需要*的字节数

  va_list args_copy; // 我们需要复制 va_list，因为 vsnprintf 会消耗它
  va_copy(args_copy, args);

  int needed = vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

  if (needed < 0) {
    // 格式化错误
    return false;
  }
  size_t needed_len = (size_t)needed;

  // --- 2. 确保容量 ---
  if (!string_grow(s, needed_len)) {
    return false; // OOM
  }

  // --- 3. 第二次调用 vsnprintf (真正写入) ---
  // (我们直接写入 `s->data` 缓冲区的末尾)
  char *write_ptr = s->data + s->count;
  size_t remaining_capacity = s->capacity - s->count;

  // (我们知道我们有足够空间，所以 vsnprintf 不会截断)
  int written = vsnprintf(write_ptr, remaining_capacity + 1, fmt, args);

  if (written < 0 || (size_t)written != needed_len) {
    // (理论上不应该发生，除非格式化再次出错)
    return false;
  }

  // --- 4. 更新 state (关键) ---
  s->count += needed_len;
  // (vsnprintf 已经自动写入了 \0，所以我们不需要 s->data[s->count] = '\0')

  return true;
}

/**
 * @brief (公共 API) 格式化追加
 */
bool string_append_fmt(string_t *s, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  bool ok = string_append_vfmt(s, fmt, args);
  va_end(args);
  return ok;
}

void string_clear(string_t *s) {
  s->count = 0;
  if (s->data) {
    s->data[0] = '\0';
  }
}

const char *string_as_cstr(const string_t *s) { return s->data; }

str_slice_t string_as_slice(const string_t *s) {
  return (str_slice_t){.ptr = s->data, .len = s->count};
}

size_t string_count(const string_t *s) { return s->count; }