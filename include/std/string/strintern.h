#pragma once

#include <core/mem/allocer.h>     // 依赖 vtable
#include <std/string/str_slice.h> // 依赖切片
#include <stdbool.h>
#include <stddef.h> // for size_t

/**
 * @brief 字符串驻留器
 *
 * 这是一个 "string -> unique const char*" 的集合。
 * 它接收 `str_slice_t`，并返回一个唯一的、保证
 * 以 '\0' 结尾的 `const char*`。
 */
typedef struct strintern
{
  allocer_t *alc;
  const char **entries; // 哈希表槽位 (一个 `const char*` 数组)
  size_t capacity;
  size_t count;
} strintern_t;
/**
 * @brief 初始化一个字符串驻留器 (例如在栈上)。
 *
 * @param interner 指向要初始化的 interner 实例的指针。
 * @param alc 用于所有内部存储的分配器 (vtable 句柄)。
 * @return true (成功) 或 false (OOM)。
 */
bool strintern_init(strintern_t *interner, allocer_t *alc, size_t initial_capacity);

/**
 * @brief 销毁 interner 的内部数据 (entries 数组)。
 *
 * @note 这 *不会* 释放 interner 结构体本身。
 * 它*不会*释放 `alc` 分配的任何字符串。
 */
void strintern_destroy(strintern_t *interner);

/**
 * @brief 驻留一个字符串切片。
 *
 * 检查字符串是否已被驻留。
 * - 如果是，返回指向*已存在*的、唯一的 `const char*` 指针。
 * - 如果否，使用 interner 的分配器创建*一个新的、以 '\0' 结尾*的
 * 副本，存储它，然后返回指向该新副本的指针。
 *
 * @param interner 驻留器实例。
 * @param slice 要驻留的切片 (不需要以 '\0' 结尾)。
 * @return 唯一的、持久化的 `const char*` 指针，或 OOM 时返回 NULL。
 */
const char *strintern_intern_slice(strintern_t *interner, str_slice_t slice);

/**
 * @brief (便捷函数) 驻留一个 C 字符串。
 *
 * 只是 `strintern_intern_slice(interner, slice_from_cstr(str))` 的封装。
 */
const char *strintern_intern_cstr(strintern_t *interner, const char *str);

/**
 * @brief 清除 interner 中的所有条目，但不释放其容量。
 *
 * @note 这 *不会* 释放字符串内存 (这仍然是 `alc` 的责任)。
 * 这用于在两次编译运行之间重置 interner，
 * 并且*必须*与 `bump_reset(arena)` 配合使用。
 */
void strintern_clear(strintern_t *interner);

/**
 * @brief 获取 interner 中的条目数。
 */
size_t strintern_count(const strintern_t *interner);