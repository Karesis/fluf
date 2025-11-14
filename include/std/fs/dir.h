#pragma once

#include <core/mem/allocer.h>
#include <stdbool.h>

/**
 * @brief 描述 `dir_walk` 找到的条目类型
 */
typedef enum {
  DIR_ENTRY_FILE,
  DIR_ENTRY_DIR,
  DIR_ENTRY_OTHER // (symlinks, sockets, etc.)
} dir_entry_type_t;

/**
 * @brief dir_walk 的回调函数签名。
 *
 * @param full_path *稳定的*、以 '\0' 结尾的完整路径 (例如 "src/std/file.c")。
 * (此字符串由 `alc` 分配，生命周期与 Arena 相同)。
 * @param type 条目的类型 (文件, 目录等)。
 * @param userdata 用户提供的指针，会
 * 透传。
 */
typedef void(dir_walk_callback_fn)(const char *full_path, dir_entry_type_t type,
                                   void *userdata);

/**
 * @brief (核心) 递归地遍历一个目录。
 *
 * 这封装了所有 `dirent.h` 和 `stat` 逻辑。
 * 它使用 `allocer_t` 来分配所有传递给回调的 `full_path` 字符串。
 *
 * @param alc 分配器 (Arena) 用于所有临时和回调的路径分配。
 * @param base_path 要开始遍历的目录。
 * @param cb 每个条目（文件或目录）的回调函数。
 * @param userdata 传递给回调的
 * 指针。
 * @return true (遍历成功完成) 或 false (base_path 不是目录, 或 I/O 错误)。
 */
bool dir_walk(allocer_t *alc, const char *base_path, dir_walk_callback_fn *cb,
              void *userdata);