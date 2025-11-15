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
#include <stddef.h>
#include <stdint.h>

/**
 * @brief 一个高密度位集 (在栈上初始化)
 */
typedef struct bitset {
  allocer_t *alc;
  size_t num_bits;
  size_t num_words;
  uint64_t *words;
} bitset_t;

/**
 * @brief 初始化一个新的、所有位都为 0 的位集
 *
 * @param bs 指向要初始化的 bitset_t 实例 (例如在栈上)
 * @param num_bits 集合中所需的位数
 * @param alc 用于分配内部 words 数组的分配器
 * @return true (成功) 或 false (OOM)
 */
bool bitset_init(bitset_t *bs, size_t num_bits, allocer_t *alc);

/**
 * @brief 初始化一个新的、所有位都为 1 的位集 (全集)
 */
bool bitset_init_all(bitset_t *bs, size_t num_bits, allocer_t *alc);

/**
 * @brief 在分配器上创建一个新的、全 0 的位集。
 */
static inline bitset_t *bitset_new(size_t num_bits, allocer_t *alc) {
  bitset_t *bs = (bitset_t *)allocer_alloc(alc, layout_of(bitset_t));
  if (bs == NULL) {
    return NULL; // OOM
  }
  if (!bitset_init(bs, num_bits, alc)) {
    return NULL; // OOM
  }
  return bs;
}

/**
 * @brief 在分配器上创建一个新的、全 1 的位集。
 */
static inline bitset_t *bitset_new_all(size_t num_bits, allocer_t *alc) {
  bitset_t *bs = (bitset_t *)allocer_alloc(alc, layout_of(bitset_t));
  if (bs == NULL) {
    return NULL; // OOM
  }
  if (!bitset_init_all(bs, num_bits, alc)) {
    return NULL; // OOM
  }
  return bs;
}

/**
 * @brief 销毁位集的内部存储 (words 数组)
 * @note 这不会释放 bitset_t 结构体本身
 */
void bitset_destroy(bitset_t *bs);

/* --- (所有其他 API 保持不变，只重命名参数) --- */

void bitset_set(bitset_t *bs, size_t bit);
void bitset_clear(bitset_t *bs, size_t bit);
bool bitset_test(const bitset_t *bs, size_t bit);
void bitset_set_all(bitset_t *bs);
void bitset_clear_all(bitset_t *bs);
bool bitset_equals(const bitset_t *bs1, const bitset_t *bs2);
void bitset_copy(bitset_t *dest, const bitset_t *src);
void bitset_intersect(bitset_t *dest, const bitset_t *src1,
                      const bitset_t *src2);
void bitset_union(bitset_t *dest, const bitset_t *src1, const bitset_t *src2);
void bitset_difference(bitset_t *dest, const bitset_t *src1,
                       const bitset_t *src2);

/**
 * @brief [调试用] 统计集合中 1 的数量 (使用 Clang intrinsic，非常快)
 */
size_t bitset_count(const bitset_t *bs);