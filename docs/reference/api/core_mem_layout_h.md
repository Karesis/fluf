# core/mem/layout.h

## `typedef struct Layout {`


A memory layout descripter

Describes the size and alignment requirements for a block of memory.
Just like `std::alloc::Layout` in Rust.


---

## `static inline layout_t layout(usize size, usize align) {`


Create a `Layout` instance.
(`Layout::from_size_align`)


- **`size`**: The size of the memory (byte).
- **`align`**: The alignment of the memory (in bytes). Must be a power of two.
@panic Triggers the assertion (in debug mode) if `align` is not a power of two.


---

