# std/string/str_slice.h

## `typedef struct {`


字符串切片 (String Slice / View)

一个非拥有 (non-owning) 的 "胖指针"，代表一个*不一定*
以 '\0' 结尾的字符串视图。

它的核心优势是 `len` 是 O(1) 的，并且它可以
"切片" (slice) 现有的内存（如源文件），而无需复制。


---

## `#define SLICE_LITERAL(s) ((strslice_t){`


(辅助宏) 从 C 字符串字面量创建切片

示例: strslice_t s = SLICE_LITERAL("hello");
(s.ptr = "hello", s.len = 5)
* sizeof("hello") == 6 (包含 \0)


---

## `static inline strslice_t slice_from_cstr(const char *cstr) {`


(辅助函数) 从 C 字符串 (const char*) 创建切片
* 这是一个 O(n) 操作，因为它调用了 strlen。


---

## `static inline bool slice_equals(strslice_t a, strslice_t b) {`


(辅助函数) 比较两个切片是否相等 (O(n))


---

## `static inline bool slice_equals_cstr(strslice_t a, const char *b_cstr) {`


(辅助函数) 比较切片和一个 C 字符串是否相等 (O(n))


---

