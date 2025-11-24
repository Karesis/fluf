# std/strings/string.h

## `typedef struct String {`


Dynamic String (Owned).

A growable, mutable string buffer that owns its memory.

Invariants:
1. Always null-terminated (data[len] == '\0'), even if empty.
2. Capacity >= len + 1.


---

## `[[nodiscard]] bool string_init(string_t *s, allocer_t alc, usize cap_hint);`


Initialize a string on the stack.

- **`cap_hint`**: Initial capacity (excluding null). 0 is allowed (lazy alloc).


---

## `void string_deinit(string_t *s);`


Free the string's internal buffer.


---

## `[[nodiscard]] string_t *string_new(allocer_t alc, usize cap_hint);`


Create a new string on the heap/arena.

- **Returns**: Pointer to string_t, or nullptr on OOM.


---

## `void string_drop(string_t *s);`


Destroy a heap-allocated string.


---

## `[[nodiscard]] string_t *string_from_str(allocer_t alc, str_t s);`


Create a string from a slice (Clone).


---

## `[[nodiscard]] string_t *string_from_cstr(allocer_t alc, const char *cstr);`


Create a string from a C-string (Clone).


---

## `[[nodiscard]] bool string_push(string_t *s, char c);`


Append a single character.


---

## `[[nodiscard]] bool string_append(string_t *s, str_t slice);`


Append a string slice (str_t).
Use this for most append operations (it handles C-strings too via macros/casting).


---

## `[[nodiscard]] bool string_append_cstr(string_t *s, const char *cstr);`


Append a C-string.


---

## `[[nodiscard]] bool string_fmt(string_t *s, const char *fmt, ...);`


Append formatted data (printf style).


---

## `void string_clear(string_t *s);`


Clear the string content (len = 0), keep capacity.
Ensures data[0] == '\0'.


---

## `[[nodiscard]] bool string_reserve(string_t *s, usize additional);`


Reserve capacity for at least `additional` bytes.


---

## `static inline usize string_len(const string_t *s) {`


Get the length (strlen).


---

## `static inline bool string_is_empty(const string_t *s) {`


Check if empty.


---

## `static inline str_t string_as_str(const string_t *s) {`


View as a slice (str_t).

> **Note:** The view is valid only until the string is modified.


---

## `static inline const char *string_cstr(const string_t *s) {`


View as a C-string.

> **Note:** The pointer is valid only until the string is modified.


---

