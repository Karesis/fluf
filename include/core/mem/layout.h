#pragma once

#include <core/msg/asrt.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 内存请求描述符 (Memory Request Descriptor)
 *
 * 描述了一个内存块的大小和对齐要求。
 * 这等同于 Rust 的 `std::alloc::Layout`。
 */
typedef struct layout {
  size_t size;
  size_t align;
} layout_t;

/**
 * @brief 辅助函数：检查 align 是否是 2 的幂
 */
static inline bool _is_power_of_two(size_t n) {
  return (n > 0) && ((n & (n - 1)) == 0);
}

/**
 * @brief 从显式的大小和对齐创建一个 Layout (Rust:
 * `Layout::from_size_align`)
 *
 * @param size 内存块的大小（字节）。
 * @param align 内存块的对齐（字节）。必须是 2 的幂。
 * @return layout_t 描述符。
 * @panic 如果 align 不是 2 的幂，触发 assert。
 */
static inline layout_t layout_from_size_align(size_t size, size_t align) {
  asrt_msg(_is_power_of_two(align), "Layout alignment must be a power of two");
  return (layout_t){.size = size, .align = align};
}

/**
 * @brief 宏：为单个类型 T 创建 Layout (Rust:
 * `Layout::new::<T>`)
 *
 * @param T 要为其创建布局的类型。
 * @return Layout 描述符。
 */
#define layout_of(T) (layout_from_size_align(sizeof(T), alignof(T)))

/**
 * @brief 宏：为一个包含 N 个 T 元素的数组创建 Layout
 * (Rust: `Layout::array::<T>(N)`)
 *
 * @param T 元素类型。
 * @param N 元素数量。
 * @return Layout 描述符。
 */
#define layout_of_array(T, N)                                                  \
  (layout_from_size_align(sizeof(T) * (N), alignof(T)))
