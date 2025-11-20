# core/span.h

## `struct Span {`


[start, end)


---

## `static inline span_t span(usize start, usize end) {`


Get a struct Span.


> **Note:** if start > end, it will return a Struct Span 
with start == end (empty span)


---

## `static inline span_t span_from_len(usize start, usize len) {`


Get span from [start, start + len)


---

## `static inline bool span_cmp(span_t a, span_t b) {`


Compare if two span are equal.


---

## `static inline span_t span_merge(span_t a, span_t b) {`


Merge two spans


**Example:**

```c
 * /// for assert
 * #include <assert.h>
 *
 * auto a = span(0, 10);
 * auto b = span(10, 16);
 * auto span = span_merge(a, b);
 * assert(span_cmp(span, span(0, 16)));
 
```

---

## `static inline usize span_len(span_t span) {`


Get the length of the span.


---

## `#define for_span(var, span) \ static_assert(is_type(span, span_t), \ "for_span_in: argument must be of type 'span_t'");`


Iter a span.


> **Note:** This macro is type-safe and ensures the second argument
is exactly of type `span_t` at compile-time.


**Example:**

```c
 * /// iter from 0 to 9
 * for_span_in(i, span(0, 10))
 * {
 * dbg("i = %zu", i);
 * }
 
```

---

## `#define foreach(var, start, end) \ static_assert( \ typecmp(start, end), \ "foreach: 'start' and 'end' must have compatible types");`


Iter from start to end.


> **Note:** This macro is type-safe. It uses `typeof` to infer the iterator's
type from `start` and uses `static_assert` to ensure `start` and `end`
types are compatible.


**Example:**

```c
 * /// iter from 5 to 9
 * foreach(i, 5, 10)
 * {
 *	dbg("i = %zu", i);
 * }
 *
 * /// iter from -5 to -1
 * foreach(i, -5, 0)
 * {
 *	dbg("i = %d", i);
 * }
 
```

---

