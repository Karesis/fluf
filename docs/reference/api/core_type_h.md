# core/type.h

## `#define uptr(x) ((uintptr_t)(x)) /// --- Floating Point --- #define f32(x) ((float)(x)) #define f64(x) ((double)(x)) /// === Type Definitions === /// unsigned int typedef uint8_t u8;`


Construct a uintptr_t (unsigned pointer-sized int).
Used for pointer arithmetic or bitwise operations on addresses.


---

## `static inline bool _safe_str_eq(const char *a, const char *b) {`


Internal helper for safe string comparison.
Handles nullptr pointers gracefully: nullptr == nullptr, nullptr != "str".


---

