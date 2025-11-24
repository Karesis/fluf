# std/strings/str.h

## `typedef struct Str {`


String Slice (String View).

A non-owning "fat pointer" representing a string that is NOT necessarily
null-terminated.

Advantages:
1. O(1) length retrieval.
2. Cheap to copy (just two integers).
3. Can reference substrings without memory allocation.


---

## `#define str(literal) ((str_t){`


Create a slice from a string literal.


> **Note:** sizeof("lit") includes the null terminator, so we subtract 1.


---

## `static inline str_t str_from_cstr(const char *cstr) {`


Create a slice from a C-style string (const char*).

> **Note:** O(n) operation (calls strlen).


---

## `static inline str_t str_from_parts(const char *ptr, usize len) {`


Create a slice from pointer and length.


> **Note:** This function trusts the caller!
It assumes `ptr` points to at least `len` readable bytes.
C cannot verify this at runtime without heavy tooling (like ASan).


---

## `static inline bool str_is_empty(str_t s) {`


Check if the slice is empty.


---

## `static inline bool str_eq(str_t a, str_t b) {`


Compare two slices for equality.


---

## `static inline bool str_eq_cstr(str_t a, const char *b_cstr) {`


Compare slice with a C-string.


---

## `static inline Ordering str_cmp(str_t a, str_t b) {`


Compare two slices lexicographically (like strcmp).

- **Returns**: < 0 if a < b, 0 if a == b, > 0 if a > b


---

## `static inline bool str_starts_with(str_t s, str_t prefix) {`


Check if slice starts with a prefix.


---

## `static inline bool str_ends_with(str_t s, str_t suffix) {`


Check if slice ends with a suffix.


---

## `static inline str_t str_trim_left(str_t s) {`


Trim whitespace from the start.


---

## `static inline str_t str_trim_right(str_t s) {`


Trim whitespace from the end.


---

## `static inline str_t str_trim(str_t s) {`


Trim whitespace from both ends.


---

## `static inline bool str_split(str_t *input, char delim, str_t *out_chunk) {`


Iterator: Split slice by a delimiter character.


- **`input`**: [in/out] The slice to split. Will be updated to point to the remainder.
- **`delim`**: The delimiter character.
- **`out_chunk`**: [out] The extracted substring.
- **Returns**: true if a chunk was found, false if iteration is finished.


**Example:**

```c
 * str_t s = str("a,b");
 * str_t chunk;
 * while (str_split(&s, ',', &chunk)) { ... }
 
```

---

## `static inline bool str_split_line(str_t *input, str_t *out_line) {`


Iterator: Split slice by lines (handles \n and \r\n).

Logic differences from str_split:
1. It treats '\r\n' as a single newline.
2. It does NOT emit an empty string if the input ends with a newline.
3. It returns false immediately if input is empty.


- **Returns**: true if a line was read, false if EOF.


---

## `#define str_for_split(var, src, delim) \ for (str_t _state_##var = (src), var;`


Iterate over a slice by splitting it.


- **`var`**: The name of the loop variable (type: str_t).
This variable is declared automatically within the loop scope.

- **`src`**: The source slice to iterate over (str_t).
Note: The source is NOT modified (a copy is used for iteration).

- **`delim`**: The delimiter character (char).


**Example:**

```c
 * str_t s = str("apple,banana,orange");
 * str_for_split(fruit, s, ',') {
 * // fruit is str_t
 * printf("Item: " fmt(str) "\n", fruit);
 * }
 
```

---

## `#define str_for_lines(line, src) \ for (str_t _state_##line = (src), line;`


Iterate over lines in a slice.
Handles Unix (\n) and Windows (\r\n) line endings automatically.


---

## `#define str_for_each(var, src) \ for (usize _i_##var = 0;`


Iterate over bytes (chars) in a slice.


- **`var`**: The name of the loop variable (type: char).
- **`src`**: The source slice.

Logic:
1. Outer loop manages index `_i`.
2. Inner loop declares `var` AND a flag `_once`.
3. Inner loop runs exactly once because `_once` is set to 0 after first run.


---

