#include <std/string/string.h> // 你的路径
#include <core/mem/allocer.h>  // 你的路径
#include <core/mem/layout.h>   // 你的路径
#include <core/msg/asrt.h>     // 你的路径
#include <string.h>            // for strlen, memcpy, memset

// --- 内部常量 ---
#define STRING_DEFAULT_CAPACITY 8

// --- 内部辅助函数 ---

/**
 * @brief 内部扩容函数
 * * 确保有足够的空间容纳 `needed` 个*额外*的字节，
 * * 并始终保留 `\0` 的空间。
 * @return true (成功) 或 false (OOM)
 */
static bool
string_grow(string_t *s, size_t needed)
{
  size_t new_count = s->count + needed;
  if (new_count <= s->capacity)
  {
    return true; // 空间足够
  }

  // 1. 计算新容量
  size_t new_capacity = s->capacity * 2;
  if (new_capacity == 0)
    new_capacity = STRING_DEFAULT_CAPACITY;
  while (new_capacity < new_count)
  {
    new_capacity *= 2;
  }

  // 2. 定义新旧布局 (容量 + 1 是为 \0)
  layout_t old_layout = layout_of_array(char, s->capacity + 1);
  layout_t new_layout = layout_of_array(char, new_capacity + 1);

  // 3. 重新分配
  char *new_data;
  if (s->capacity == 0)
  {
    new_data = allocer_alloc(s->alc, new_layout);
  }
  else
  {
    new_data = allocer_realloc(s->alc, s->data, old_layout, new_layout);
  }

  if (!new_data)
  {
    return false; // OOM
  }

  // 4. 更新 string
  s->data = new_data;
  s->capacity = new_capacity;
  return true;
}

// --- 公共 API 实现 ---

bool string_init(string_t *s, allocer_t *alc, size_t initial_capacity)
{
  asrt_msg(s != NULL, "String pointer cannot be NULL");
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  size_t capacity = (initial_capacity > 0) ? initial_capacity : STRING_DEFAULT_CAPACITY;

  // 1. 分配初始内存 (容量 + 1 用于 \0)
  layout_t layout = layout_of_array(char, capacity + 1);
  char *data = allocer_alloc(alc, layout);

  if (!data)
  {
    memset(s, 0, sizeof(string_t));
    return false; // OOM
  }

  // 2. 初始化结构体
  s->alc = alc;
  s->data = data;
  s->capacity = capacity;
  s->count = 0;

  // 3. (关键) 设置初始的 \0
  s->data[0] = '\0';

  return true;
}

void string_destroy(string_t *s)
{
  if (!s)
    return;

  if (s->data)
  {
    // 释放 `data` 缓冲区
    layout_t layout = layout_of_array(char, s->capacity + 1);
    allocer_free(s->alc, s->data, layout);
  }

  // 清理结构体
  memset(s, 0, sizeof(string_t));
}

bool string_push(string_t *s, char c)
{
  // 1. 确保有空间 (1 个 char)
  if (!string_grow(s, 1))
  {
    return false; // OOM
  }

  // 2. 插入
  s->data[s->count] = c;
  s->count++;

  // 3. (关键) 维护 \0
  s->data[s->count] = '\0';

  return true;
}

bool string_append_cstr(string_t *s, const char *cstr)
{
  size_t len = strlen(cstr);
  if (len == 0)
    return true; // (什么也不做)

  // 1. 确保有空间 (len 个 char)
  if (!string_grow(s, len))
  {
    return false; // OOM
  }

  // 2. 插入 (memcpy)
  memcpy(s->data + s->count, cstr, len);
  s->count += len;

  // 3. (关键) 维护 \0
  s->data[s->count] = '\0';

  return true;
}

bool string_append_slice(string_t *s, str_slice_t slice)
{
  if (slice.len == 0)
    return true; // (什么也不做)

  // 1. 确保有空间 (slice.len 个 char)
  if (!string_grow(s, slice.len))
  {
    return false; // OOM
  }

  // 2. 插入 (memcpy)
  memcpy(s->data + s->count, slice.ptr, slice.len);
  s->count += slice.len;

  // 3. (关键) 维护 \0
  s->data[s->count] = '\0';

  return true;
}

void string_clear(string_t *s)
{
  s->count = 0;
  if (s->data)
  {
    s->data[0] = '\0'; // 维护 \0
  }
}

const char *
string_as_cstr(const string_t *s)
{
  // O(1), 并且保证 \0 结尾
  return s->data;
}

str_slice_t
string_as_slice(const string_t *s)
{
  return (str_slice_t){.ptr = s->data, .len = s->count};
}

size_t
string_count(const string_t *s)
{
  return s->count;
}