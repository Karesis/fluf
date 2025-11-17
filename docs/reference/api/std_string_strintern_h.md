# std/string/strintern.h

## `typedef struct strintern {`


字符串驻留器

这是一个 "string -> unique const char*" 的集合。
它接收 `strslice_t`，并返回一个唯一的、保证
以 '\0' 结尾的 `const char*`。


---

## `bool strintern_init(strintern_t *interner, allocer_t *alc, size_t initial_capacity);`


初始化一个字符串驻留器 (例如在栈上)。


- **`interner`**: 指向要初始化的 interner 实例的指针。
- **`alc`**: 用于所有内部存储的分配器 (vtable 句柄)。
- **Returns**: true (成功) 或 false (OOM)。


---

## `void strintern_destroy(strintern_t *interner);`


销毁 interner 的内部数据 (entries 数组)。


> **Note:** 这 *不会* 释放 interner 结构体本身。
它*不会*释放 `alc` 分配的任何字符串。


---

## `const char *strintern_intern_slice(strintern_t *interner, strslice_t slice);`


驻留一个字符串切片。

检查字符串是否已被驻留。
- 如果是，返回指向*已存在*的、唯一的 `const char*` 指针。
- 如果否，使用 interner 的分配器创建*一个新的、以 '\0' 结尾*的
副本，存储它，然后返回指向该新副本的指针。


- **`interner`**: 驻留器实例。
- **`slice`**: 要驻留的切片 (不需要以 '\0' 结尾)。
- **Returns**: 唯一的、持久化的 `const char*` 指针，或 OOM 时返回 NULL。


---

## `const char *strintern_intern_cstr(strintern_t *interner, const char *str);`


(便捷函数) 驻留一个 C 字符串。

只是 `strintern_intern_slice(interner, slice_from_cstr(str))` 的封装。


---

## `void strintern_clear(strintern_t *interner);`


清除 interner 中的所有条目，但不释放其容量。


> **Note:** 这 *不会* 释放字符串内存 (这仍然是 `alc` 的责任)。
这用于在两次编译运行之间重置 interner，
并且*必须*与 `bump_reset(arena)` 配合使用。


---

## `size_t strintern_count(const strintern_t *interner);`


获取 interner 中的条目数。


---

