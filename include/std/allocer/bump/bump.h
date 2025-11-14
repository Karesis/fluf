#pragma once

#include <core/mem/layout.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * * 内存布局:
 * [------------------ data ------------------][-- ChunkFooter --]
 * ^                                          ^                 ^
 * |                                          |                 |
 * data                                       ptr         (data + chunk_size)
 * (由 malloc 返回)                         (碰撞指针)
 *
 * 碰撞指针(ptr) 从 (data + chunk_size - sizeof(ChunkFooter)) 开始，
 * 向 data 方向（低地址）移动。
 */
typedef struct chunkfooter chunkfooter_t;
struct chunkfooter {
  unsigned char *data;
  size_t chunk_size;
  chunkfooter_t *prev;
  unsigned char *ptr;
  size_t allocated_bytes;
};

/*
 * 对应 Rust 的 bump_t
 */
typedef struct bump {

  chunkfooter_t *current_chunk_footer;

  size_t allocation_limit;

  size_t min_align;
} bump_t;

/*
 * --- 生命周期 ---
 */

/**
 * @brief 在堆上创建一个新的 bump_t Arena。
 * * @param min_align 最小对齐。所有分配都将至少以此对齐。
 * 必须是 2 的幂，且不大于 16。
 * 传 1 表示默认对齐。
 * @return bump_t* 成功则返回指向新 Arena 的指针，失败返回 NULL。
 * @note 用户必须调用 bump_free() 来释放。
 */
bump_t *bump_new_with_min_align(size_t min_align);

/**
 * @brief 在堆上创建一个新的 bump_t Arena (最小对齐为 1)。
 *
 * @return bump_t* 成功则返回指向新 Arena 的指针，失败返回 NULL。
 * @note 用户必须调用 bump_free() 来释放。
 */
bump_t *bump_new(void);

/**
 * @brief 初始化一个已分配的 bump_t 结构 (例如在栈上)。
 * @param bump 指向要初始化的 bump_t 实例的指针。
 * @param min_align 最小对齐。
 */
void bump_init_with_min_align(bump_t *bump, size_t min_align);

/**
 * @brief 初始化一个已分配的 bump_t 结构 (最小对齐为 1)。
 * * @param bump 指向要初始化的 bump_t 实例的指针。
 */
void bump_init(bump_t *bump);

/**
 * @brief 销毁 Arena，释放其分配的所有内存块。
 * @note 这 *不会* 释放 bump 结构体本身。
 * 这允许 bump_init/bump_destroy 用于栈分配的 Arena。
 *
 * @param bump 要销毁的 Arena。
 */
void bump_destroy(bump_t *bump);

/**
 * @brief 销毁 Arena 并释放 bump_t 结构体本身。
 * @note 只能用于由 bump_new() 创建的 Arena。
 *
 * @param bump 要释放的 Arena。
 */
void bump_free(bump_t *bump);

/**
 * @brief 重置 Arena。
 *
 * 释放所有已分配的 Chunk (除了当前 Chunk)，
 * 并将当前 Chunk 的碰撞指针重置。
 * 之后可以重用 Arena。
 *
 * @param bump 要重置的 Arena。
 */
void bump_reset(bump_t *bump);

/*
 * --- 分配 API ---
 */

/**
 * @brief 核心分配函数。
 *
 * 在 Arena 中分配一块具有指定布局 (大小和对齐) 的内存。
 * 内存是 *未初始化* 的。
 *
 * @param bump Arena。
 * @param layout 内存布局 (大小和对齐)。
 * @return void* 成功则返回指向已分配内存的指针，失败 (OOM) 返回 NULL。
 */
void *bump_alloc_layout(bump_t *bump, layout_t layout);

/**
 * @brief 便捷函数：按大小和对齐分配。
 *
 * @param bump Arena。
 * @param size 要分配的字节数。
 * @param align 内存对齐 (必须是 2 的幂)。
 * @return void* 成功则返回指针，失败返回 NULL。
 */
void *bump_alloc(bump_t *bump, size_t size, size_t align);

/**
 * @brief 分配一块清零的内存。
 */
void *bump_alloc_layout_zeroed(bump_t *bump, layout_t layout);

/**
 * @brief 便捷函数：分配并复制数据。(memcpy风格)
 *
 * @param bump Arena。
 * @param src 要复制的数据源。
 * @param size 要分配和复制的字节数。
 * @param align 内存对齐 (必须是 2 的幂)。
 * @return void* 成功则返回指向 Arena 中新副本的指针，失败返回 NULL。
 */
void *bump_alloc_copy(bump_t *bump, const void *src, size_t size, size_t align);

/**
 * @brief 便捷函数：分配并复制一个字符串。
 *
 * @param bump Arena。
 * @param str 要复制的 C 字符串 (以 '\0' 结尾)。
 * @return char* 成功则返回指向 Arena 中新字符串的指针，失败返回 NULL。
 */
char *bump_alloc_str(bump_t *bump, const char *str);

/**
 * @brief 重新分配一块内存 (分配 + 复制)。
 *
 * 在 arena 中，这*几乎*总是会分配一块 *新* 内存，
 * 复制 [old_ptr, old_ptr + old_size] 的内容，
 * 并 "泄露" (abandon) 旧的 [old_ptr] 内存块。
 *
 * @param bump Arena。
 * @param old_ptr 指向要 "重新分配" 的旧内存块。如果为 NULL，则等同于
 * bump_alloc。
 * @param old_size *旧内存块中要复制的数据大小*。
 * @param new_size *要分配的新内存块的总大小*。
 * @param align 新内存块的对齐方式。
 * @return void* 成功则返回指向*新*内存块的指针，失败返回 NULL。
 */
void *bump_realloc(bump_t *bump, void *old_ptr, size_t old_size,
                   size_t new_size, size_t align);

/*
 * --- 容量和限制 ---
 */

/**
 * @brief 设置分配上限 (字节)。
 *
 * @param bump Arena。
 * @param limit 新的上限。传入 SIZE_MAX 表示无限制。
 */
void bump_set_allocation_limit(bump_t *bump, size_t limit);

/**
 * @brief 获取当前已分配的 *可用* 字节总数 (所有 Chunk 的容量总和)。
 *
 * @param bump Arena。
 * @return size_t 字节数。
 */
size_t bump_get_allocated_bytes(bump_t *bump);

/**
 * @brief 分配单个 T 实例
 *
 * @param bump_ptr (bump_t *) Arena 指针。
 * [!!!] 注意：传入 (bump_t *)，不要传入 (bump_t **)。
 * 例如：BUMP_ALLOC(my_arena, ...)  [正确]
 * BUMP_ALLOC(&my_arena, ...) [错误]
 * @param T 要分配的类型
 */
#define BUMP_ALLOC(bump_ptr, T)                                                \
  ((T *)bump_alloc_layout((bump_ptr), layout_of(T)))

#define BUMP_ALLOC_SLICE(bump_ptr, T, count)                                   \
  ((T *)bump_alloc_layout((bump_ptr), layout_of_array(T, (count))))

#define BUMP_ALLOC_SLICE_COPY(bump_ptr, T, src_ptr, count)                     \
  ((T *)bump_alloc_copy((bump_ptr), (src_ptr), sizeof(T) * (count), alignof(T)))

#define BUMP_ALLOC_ZEROED(bump_ptr, T)                                         \
  ((T *)bump_alloc_layout_zeroed((bump_ptr), layout_of(T)))

#define BUMP_ALLOC_SLICE_ZEROED(bump_ptr, T, count)                            \
  ((T *)bump_alloc_layout_zeroed((bump_ptr), layout_of_array(T, (count))))

/**
 * @brief 重新分配 T 的数组（分配 + 复制）。
 *
 * @param bump_ptr Arena 指针。
 * @param T 元素类型。
 * @param old_ptr 指向旧的 slice。
 * @param old_count 要复制的 *元素* 数量 (来自旧 slice)。
 * @param new_count 要分配的 *元素* 数量 (用于新 slice)。
 */
#define BUMP_REALLOC_SLICE(bump_ptr, T, old_ptr, old_count, new_count)         \
  ((T *)bump_realloc((bump_ptr), (old_ptr), sizeof(T) * (old_count),           \
                     sizeof(T) * (new_count), alignof(T)))
