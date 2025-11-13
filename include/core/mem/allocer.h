#pragma once

#include <core/mem/layout.h>

// "allocer_t" 是一个不透明的 "胖指针" 句柄
// 它包含指向 vtable 的指针和指向实例数据(self)的指针
typedef struct allocer_t allocer_t;

/**
 * @brief 分配器虚函数表 (The V-Table)
 *
 * 定义了分配器必须实现的函数"契约"。
 * 每个函数都接收一个 `void* self` 指针，指向具体的分配器实例
 * (例如 bump_t* 或 system_alloc_t*)。
 */
typedef struct allocer_vtable
{
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
    void *(*realloc)(
        void *self,
        void *ptr,
        layout_t old_layout,
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
struct allocer_t
{
    void *self;
    const allocer_vtable_t *vtable;
};

/* --- API 辅助函数 --- */

/**
 * @brief (API) 通过 vtable 分配内存
 */
static inline void *
allocer_alloc(allocer_t *alc, layout_t layout)
{
    // 简单的 vtable 调用：可调试，易于理解
    return alc->vtable->alloc(alc->self, layout);
}

/**
 * @brief (API) 通过 vtable 释放内存
 */
static inline void
allocer_free(allocer_t *alc, void *ptr, layout_t layout)
{
    alc->vtable->free(alc->self, ptr, layout);
}

/**
 * @brief (API) 通过 vtable 重新分配内存
 */
static inline void *
allocer_realloc(
    allocer_t *alc,
    void *ptr,
    layout_t old_layout,
    layout_t new_layout)
{
    return alc->vtable->realloc(alc->self, ptr, old_layout, new_layout);
}

/**
 * @brief (API) 通过 vtable 分配清零的内存
 */
static inline void *
allocer_zalloc(allocer_t *alc, layout_t layout)
{
    return alc->vtable->zalloc(alc->self, layout);
}