#pragma once

#include <core/mem/allocer.h> // 依赖 vtable
#include <core/mem/layout.h>  // 依赖 layout_of
#include <core/msg/asrt.h>    // 依赖 asrt
#include <stddef.h>           // 依赖 size_t
#include <string.h>           // 依赖 strlen, memcpy

/**
 * @brief (核心) 复制一个 C-string 到分配器中。
 *
 * 这是一个安全的 "strdup" 版本，它使用 fluf 的分配器。
 * 它分配 (strlen(cstr) + 1) 字节，复制字符串，
 * 并返回一个指向*新*内存的指针。
 *
 * 这是解决 `string_t` 悬垂指针陷阱的首选方案。
 *
 * @param alc 用于分配新字符串的分配器。
 * @param cstr 要复制的、以 '\0' 结尾的字符串。
 * @return 一个指向新副本的 `char*`，或 OOM 时返回 NULL。
 */
static inline char *allocer_strdup(allocer_t *alc, const char *cstr) {
  asrt_msg(alc != NULL, "Allocator cannot be NULL");
  if (cstr == NULL) {
    return NULL;
  }

  size_t len = strlen(cstr);
  layout_t layout = layout_of_array(char, len + 1);

  char *new_str = (char *)allocer_alloc(alc, layout);
  if (new_str == NULL) {
    return NULL; // OOM
  }

  // +1 复制 \0
  memcpy(new_str, cstr, len + 1);
  return new_str;
}