# core/span.h

## `typedef struct span {`


(结构体) 表示一个半开半闭区间 [start, end)


---

## `static inline span_t range(size_t start, size_t end) {`


(构造函数) 创建一个 span_t 结构体


> **Note:** 如果 start > end, 结果会是一个空范围 (start ==
end)。


---

## `static inline span_t span_from_len(size_t start, size_t len) {`


(构造函数) 从 [start, start + len) 范围创建一个 span
(这个在词法分析器 (Lexer) 中非常有用)


---

## `static inline span_t span_merge(span_t a, span_t b) {`


(辅助函数) 合并两个 span

(这个在解析器 (Parser) 中至关重要，
例如 `(a + b)` 的 span = `span_merge(a.span, b.span)`)


---

## `static inline size_t span_len(span_t span) {`


(辅助函数) 获取 span 的长度


---

## `#define for_range(var, start, end) \ for (size_t var = (start);`


(宏) 遍历一个字面量范围 [start, end)


**Example:**

```c
 * for_range(i, 0, 10) { // i 将从 0 到 9
 *   dbg("i = %zu", i);
 *  }
 
```

---

## `#define for_range_in(var, range_obj) \ for (size_t var = (range_obj).start;`


(宏) 遍历一个 span_t 结构体


**Example:**

```c
 * span_t r = range(5, 10);
 * for_range_in(i, r) { // i 将从 5 到 9
 *  dbg("i = %zu", i);
 * }
 
```

---

