# core/mem/layout.h

## `typedef struct layout {`


内存请求描述符 (Memory Request Descriptor)

描述了一个内存块的大小和对齐要求。
这等同于 Rust 的 `std::alloc::Layout`。


---

## `static inline bool _is_power_of_two(size_t n) {`


辅助函数：检查 align 是否是 2 的幂


---

## `static inline layout_t layout_from_size_align(size_t size, size_t align) {`


从显式的大小和对齐创建一个 Layout (Rust:
`Layout::from_size_align`)


- **`size`**: 内存块的大小（字节）。
- **`align`**: 内存块的对齐（字节）。必须是 2 的幂。
- **Returns**: layout_t 描述符。
@panic 如果 align 不是 2 的幂，触发 assert。


---

