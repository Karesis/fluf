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

#include <core/mem/layout.h>

typedef struct allocer_t allocer_t;

/**
 * @brief 分配器虚函数表 (The V-Table)
 *
 * 定义了分配器必须实现的函数"契约"。
 * 每个函数都接收一个 `void* self` 指针，指向具体的分配器实例
 * (例如 bump_t* 或 system_alloc_t*)。
 */
typedef struct allocer_vtable {
  /** * 分配内存。
   * 成功时返回一个指针，OOM 时返回 NULL。
   */
  void *(*alloc)(void *self, layout_t layout);

  /** * 释放内存。
   * 注意：对于 Arena，这通常是一个 no-op (什么也不做)。
   */
  void (*free)(void *self, void *ptr, layout_t layout);

  /** * 重新分配内存。
   * 成功时返回一个新指针，OOM 时返回 NULL。
   */
  void *(*realloc)(void *self, void *ptr, layout_t old_layout,
                   layout_t new_layout);

  /** * 分配清零的内存。
   * 成功时返回一个指针，OOM 时返回 NULL。
   */
  void *(*zalloc)(void *self, layout_t layout);

} allocer_vtable_t;

/**
 * @brief 分配器句柄 (The "Fat Pointer")
 *
 * 这是用户持有的对象。它将"实例数据"(self)和"实现"(vtable)捆绑在一起。
 */
struct allocer_t {
  void *self;
  const allocer_vtable_t *vtable;
};

/* --- API 辅助函数 --- */

/**
 * @brief (API) 通过 vtable 分配内存
 */
static inline void *allocer_alloc(allocer_t *alc, layout_t layout) {

  return alc->vtable->alloc(alc->self, layout);
}

/**
 * @brief (API) 通过 vtable 释放内存
 */
static inline void allocer_free(allocer_t *alc, void *ptr, layout_t layout) {
  alc->vtable->free(alc->self, ptr, layout);
}

/**
 * @brief (API) 通过 vtable 重新分配内存
 */
static inline void *allocer_realloc(allocer_t *alc, void *ptr,
                                    layout_t old_layout, layout_t new_layout) {
  return alc->vtable->realloc(alc->self, ptr, old_layout, new_layout);
}

/**
 * @brief (API) 通过 vtable 分配清零的内存
 */
static inline void *allocer_zalloc(allocer_t *alc, layout_t layout) {
  return alc->vtable->zalloc(alc->self, layout);
}