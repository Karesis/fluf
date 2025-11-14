# std/allocer/bump/bump.h

## `bump_t *bump_new_with_min_align(size_t min_align);`


在堆上创建一个新的 bump_t Arena。
* @param min_align 最小对齐。所有分配都将至少以此对齐。
必须是 2 的幂，且不大于 16。
传 1 表示默认对齐。

- **Returns**: bump_t* 成功则返回指向新 Arena 的指针，失败返回 NULL。

> **Note:** 用户必须调用 bump_free() 来释放。


---

## `bump_t *bump_new(void);`


在堆上创建一个新的 bump_t Arena (最小对齐为 1)。


- **Returns**: bump_t* 成功则返回指向新 Arena 的指针，失败返回 NULL。

> **Note:** 用户必须调用 bump_free() 来释放。


---

## `void bump_init_with_min_align(bump_t *bump, size_t min_align);`


初始化一个已分配的 bump_t 结构 (例如在栈上)。

- **`bump`**: 指向要初始化的 bump_t 实例的指针。
- **`min_align`**: 最小对齐。


---

## `void bump_init(bump_t *bump);`


初始化一个已分配的 bump_t 结构 (最小对齐为 1)。
* @param bump 指向要初始化的 bump_t 实例的指针。


---

## `void bump_destroy(bump_t *bump);`


销毁 Arena，释放其分配的所有内存块。

> **Note:** 这 *不会* 释放 bump 结构体本身。
这允许 bump_init/bump_destroy 用于栈分配的 Arena。


- **`bump`**: 要销毁的 Arena。


---

## `void bump_free(bump_t *bump);`


销毁 Arena 并释放 bump_t 结构体本身。

> **Note:** 只能用于由 bump_new() 创建的 Arena。


- **`bump`**: 要释放的 Arena。


---

## `void bump_reset(bump_t *bump);`


重置 Arena。

释放所有已分配的 Chunk (除了当前 Chunk)，
并将当前 Chunk 的碰撞指针重置。
之后可以重用 Arena。


- **`bump`**: 要重置的 Arena。


---

## `void *bump_alloc_layout(bump_t *bump, layout_t layout);`


核心分配函数。

在 Arena 中分配一块具有指定布局 (大小和对齐) 的内存。
内存是 *未初始化* 的。


- **`bump`**: Arena。
- **`layout`**: 内存布局 (大小和对齐)。
- **Returns**: void* 成功则返回指向已分配内存的指针，失败 (OOM) 返回 NULL。


---

## `void *bump_alloc(bump_t *bump, size_t size, size_t align);`


便捷函数：按大小和对齐分配。


- **`bump`**: Arena。
- **`size`**: 要分配的字节数。
- **`align`**: 内存对齐 (必须是 2 的幂)。
- **Returns**: void* 成功则返回指针，失败返回 NULL。


---

## `void *bump_alloc_layout_zeroed(bump_t *bump, layout_t layout);`


分配一块清零的内存。


---

## `void *bump_alloc_copy(bump_t *bump, const void *src, size_t size, size_t align);`


便捷函数：分配并复制数据。(memcpy风格)


- **`bump`**: Arena。
- **`src`**: 要复制的数据源。
- **`size`**: 要分配和复制的字节数。
- **`align`**: 内存对齐 (必须是 2 的幂)。
- **Returns**: void* 成功则返回指向 Arena 中新副本的指针，失败返回 NULL。


---

## `char *bump_alloc_str(bump_t *bump, const char *str);`


便捷函数：分配并复制一个字符串。


- **`bump`**: Arena。
- **`str`**: 要复制的 C 字符串 (以 '\0' 结尾)。
- **Returns**: char* 成功则返回指向 Arena 中新字符串的指针，失败返回 NULL。


---

## `void *bump_realloc(bump_t *bump, void *old_ptr, size_t old_size, size_t new_size, size_t align);`


重新分配一块内存 (分配 + 复制)。

在 arena 中，这*几乎*总是会分配一块 *新* 内存，
复制 [old_ptr, old_ptr + old_size] 的内容，
并 "泄露" (abandon) 旧的 [old_ptr] 内存块。


- **`bump`**: Arena。
- **`old_ptr`**: 指向要 "重新分配" 的旧内存块。如果为 NULL，则等同于
bump_alloc。

- **`old_size`**: *旧内存块中要复制的数据大小*。
- **`new_size`**: *要分配的新内存块的总大小*。
- **`align`**: 新内存块的对齐方式。
- **Returns**: void* 成功则返回指向*新*内存块的指针，失败返回 NULL。


---

## `void bump_set_allocation_limit(bump_t *bump, size_t limit);`


设置分配上限 (字节)。


- **`bump`**: Arena。
- **`limit`**: 新的上限。传入 SIZE_MAX 表示无限制。


---

## `size_t bump_get_allocated_bytes(bump_t *bump);`


获取当前已分配的 *可用* 字节总数 (所有 Chunk 的容量总和)。


- **`bump`**: Arena。
- **Returns**: size_t 字节数。


---

