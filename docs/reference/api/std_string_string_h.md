# std/string/string.h

## `typedef struct string {`


动态字符串构建器

一个有状态的、可变的字符串。它拥有自己的内存，
并保证*始终*以 '\0' 结尾。

(遵循 bump_t/interner_t/vec_t 模式, 在栈上初始化)


---

## `bool string_init(string_t *s, allocer_t *alc, size_t initial_capacity);`


初始化一个 string (例如在栈上)。


- **`s`**: 指向要初始化的 string 实例的指针。
- **`alc`**: 用于所有内部存储的分配器 (vtable 句柄)。
- **`initial_capacity`**: *不*包含 \0 的初始容量。
- **Returns**: true (成功) 或 false (OOM)。


---

## `void string_destroy(string_t *s);`


销毁 string 的内部数据 (data 数组)。


---

## `bool string_push(string_t *s, char c);`


将一个字符 (char) 追加到字符串末尾。


- **`s`**: string 实例。
- **`c`**: 要追加的字符。
- **Returns**: true (成功) 或 false (OOM / 扩容失败)。


---

## `bool string_append_cstr(string_t *s, const char *cstr);`


将一个 C-string (const char*) 追加到字符串末尾。


- **`s`**: string 实例。
- **`cstr`**: 要追加的 C 字符串 (以 \0 结尾)。
- **Returns**: true (成功) 或 false (OOM / 扩容失败)。


---

## `bool string_append_slice(string_t *s, strslice_t slice);`


将一个字符串切片 (strslice_t) 追加到字符串末尾。


- **`s`**: string 实例。
- **`slice`**: 要追加的切片 (不要求 \0 结尾)。
- **Returns**: true (成功) 或 false (OOM / 扩容失败)。


---

## `void string_clear(string_t *s);`


清空字符串 (count=0) 但保留容量。


---

## `const char *string_as_cstr(const string_t *s);`


(O(1)) 将 string 作为 C-string (const char*) 返回。
* 保证以 \0 结尾。


---

## `strslice_t string_as_slice(const string_t *s);`


(O(1)) 将 string 作为 strslice_t 返回。


---

## `size_t string_count(const string_t *s);`


(O(1)) 获取字符串长度 (strlen)。


---

