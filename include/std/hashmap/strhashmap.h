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
#include <stdbool.h>

typedef struct {
  char *key;

  void *value;
} Entry;

/**
 * @brief 字符串哈希表
 *
 * 这是一个 "const char* -> void*" 的哈希表。
 * 它使用开放寻址和线性探测。
 *
 * - 键 (const char*) 在插入时会被 *复制* 到分配器中。
 * - 值 (void*) 按原样存储。
 * - 它不拥有 `void*` 值指向的内存。
 */
typedef struct strhashmap {
  allocer_t *alc;
  Entry *entries;
  size_t capacity;
  size_t count;
} strhashmap_t;

/**
 * @brief 创建一个新的字符串哈希表。
 *
 * @param alc 用于所有内部存储的分配器 (vtable 句柄)
 * @param initial_capacity 初始容量 (槽位数)。如果为 0，将使用默认值。
 * @return 一个新的哈希表，或 OOM 时返回 NULL。
 */
strhashmap_t *strhashmap_new(allocer_t *alc, size_t initial_capacity);

/**
 * @brief 释放哈希表及其所有内部存储。
 *
 * @note 这 *不会* 释放存储在内的 `void*` 值。
 * 它会释放哈希表为 *键* (keys) 复制的字符串。
 */
void strhashmap_free(strhashmap_t *map);

/**
 * @brief 插入或更新一个键值对。
 *
 * `key` 字符串会被*复制*到哈希表的内部存储中 (使用分配器)。
 * 如果键已存在，旧的 `void*` 值将被替换 (不释放)。
 *
 * @param map 哈希表
 * @param key 要插入的 C 字符串键。
 * @param value 要关联的 `void*` 指针。
 * @return true (成功) 或 false (OOM)。
 */
bool strhashmap_put(strhashmap_t *map, const char *key, void *value);

/**
 * @brief 获取一个键关联的值。
 * @param map 哈希表
 * @param key 要查找的 C 字符串键。
 * @return 关联的 `void*` 指针，如果未找到则返回 NULL。
 * @note 如果 `NULL` 是一个有效值，请使用 `strhashmap_get_ptr`。
 */
void *strhashmap_get(strhashmap_t *map, const char *key);

/**
 * @brief (更安全的 Get) 检查键是否存在并获取值。
 *
 * @param map 哈希表
 * @param key 要查找的 C 字符串键。
 * @param out_value [out] 如果找到，值的指针将被写入这里。
 * @return true (如果找到) 或 false (如果未找到)。
 */
bool strhashmap_get_ptr(strhashmap_t *map, const char *key, void **out_value);

/**
 * @brief 从哈希表中删除一个键。
 *
 * @param map 哈希表
 * @param key 要删除的 C 字符串键。
 * @return true (如果删除成功) 或 false (如果未找到)。
 */
bool strhashmap_delete(strhashmap_t *map, const char *key);

/**
 * @brief 清除哈希表中的所有条目，但不释放其容量。
 * * 这允许为下一次编译重用符号表，而无需重新分配。
 */
void strhashmap_clear(strhashmap_t *map);

/**
 * @brief 获取哈希表中的条目数。
 */
size_t strhashmap_count(const strhashmap_t *map);

/**
 * @brief 字符串哈希表迭代器
 *
 * 用于遍历哈希表中的所有有效条目 (跳过空槽和墓碑)。
 * (在栈上初始化)
 */
typedef struct strhashmap_iter {
  const strhashmap_t *map; // 只读引用
  size_t index;            // 当前遍历到的内部数组索引
} strhashmap_iter_t;

/**
 * @brief 初始化迭代器。
 *
 * @param iter 指向栈上迭代器的指针。
 * @param map 要遍历的哈希表。
 */
void strhashmap_iter_init(strhashmap_iter_t *iter, const strhashmap_t *map);

/**
 * @brief 获取下一个条目。
 *
 * @param iter 迭代器实例。
 * @param out_key [out] 写入当前条目的 Key (如果不需要可传 NULL)。
 * @param out_value [out] 写入当前条目的 Value (如果不需要可传 NULL)。
 * @return true (还有更多条目，out_key/out_value 已更新)。
 * @return false (遍历结束)。
 *
 * @note 在遍历过程中，**不要**向哈希表中添加或删除元素 (realloc 会导致灾难)。
 * 修改 value 指向的内容是安全的。
 */
bool strhashmap_iter_next(strhashmap_iter_t *iter, const char **out_key,
                          void **out_value);