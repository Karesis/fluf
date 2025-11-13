#pragma once

#include <core/mem/allocer.h>     // 依赖 vtable
#include <std/string/str_slice.h> // 依赖切片
#include <stdbool.h>
#include <stddef.h> // for size_t

/**
 * @brief 动态字符串构建器
 *
 * 一个有状态的、可变的字符串。它拥有自己的内存，
 * 并保证*始终*以 '\0' 结尾。
 *
 * (遵循 bump_t/interner_t/vec_t 模式, 在栈上初始化)
 */
typedef struct string
{
  allocer_t *alc;
  char *data;      // 指向一个由 alc 分配的、\0 结尾的缓冲区
  size_t count;    // *不*包含 \0 的长度 (即 strlen(data))
  size_t capacity; // *不*包含 \0 的容量
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
 * @brief 清空字符串 (count=0) 但保留容量。
 */
void string_clear(string_t *s);

/**
 * @brief (O(1)) 将 string 作为 C-string (const char*) 返回。
 * * 保证以 \0 结尾。
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