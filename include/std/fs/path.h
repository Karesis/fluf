#pragma once

#include <std/string/str_slice.h> // 依赖 strslice_t
#include <std/string/string.h>    // 依赖 string_t
#include <stdbool.h>

// (在 C 语言中，我们只处理 POSIX 风格的 '/' 分隔符)
// (如果需要 Windows，我们需要在这里添加 #ifdef)
#define FLUF_PATH_SEPARATOR '/'

/**
 * @brief (核心) 安全地将两个路径组件连接到 `builder` 中。
 *
 * 自动处理 `base` 末尾和 `part` 开头的 '/'。
 * 示例:
 * - ("a", "b")     -> "a/b"
 * - ("a/", "b")    -> "a/b"
 * - ("a", "/b")    -> "a/b"
 * - ("a/", "/b")   -> "a/b"
 * - ("", "b")      -> "b"
 * - ("a", "")      -> "a"
 *
 * @param builder 要追加到的 string_t。
 * @param base 基础路径。
 * @param part 要附加的部分。
 * @return true (成功) 或 false (OOM)。
 */
static inline bool path_join(string_t *builder, strslice_t base,
                             strslice_t part) {
  // 1. 修剪 base 末尾的 '/'
  if (base.len > 0 && base.ptr[base.len - 1] == FLUF_PATH_SEPARATOR) {
    base.len--;
  }

  // 2. 修剪 part 开头的 '/'
  if (part.len > 0 && part.ptr[0] == FLUF_PATH_SEPARATOR) {
    part.ptr++;
    part.len--;
  }

  // 3. 追加
  if (base.len > 0) {
    if (!string_append_slice(builder, base))
      return false;
  }

  // 4. (关键) 只有在 base 和 part 都不为空时才添加分隔符
  if (base.len > 0 && part.len > 0) {
    if (!string_push(builder, FLUF_PATH_SEPARATOR))
      return false;
  }

  if (part.len > 0) {
    if (!string_append_slice(builder, part))
      return false;
  }

  return true;
}

/**
 * @brief (核心) 获取路径的文件扩展名。
 *
 * 示例:
 * - "main.nyan" -> ".nyan"
 * - "main."       -> "."
 * - "main"        -> "" (空切片)
 * - ".config"     -> ".config" (POSIX 风格)
 *
 * @param path 完整路径。
 * @return 一个包含扩展名（包括 '.'）的切片，或一个空切片。
 */
static inline strslice_t path_get_extension(strslice_t path) {
  // 从末尾开始查找 '.'
  for (size_t i = path.len; i > 0; i--) {
    if (path.ptr[i - 1] == '.') {
      // (我们不认为 "/.config" 中的 '.' 是扩展名)
      if (i > 1 && path.ptr[i - 2] == FLUF_PATH_SEPARATOR) {
        continue;
      }
      // 找到了!
      return (strslice_t){.ptr = path.ptr + i - 1, .len = path.len - (i - 1)};
    }
    if (path.ptr[i - 1] == FLUF_PATH_SEPARATOR) {
      // 遇到了路径分隔符，停止
      break;
    }
  }

  // 未找到
  return (strslice_t){.ptr = path.ptr + path.len, .len = 0};
}