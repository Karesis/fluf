#include <core/mem/allocer.h>
#include <core/mem/layout.h>
#include <core/msg/asrt.h>
#include <std/string/string.h>
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