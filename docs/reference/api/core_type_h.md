# core/type.h

## `static inline bool _safe_str_eq(const char *a, const char *b) {`


Internal helper for safe string comparison.
Handles nullptr pointers gracefully: nullptr == nullptr, nullptr != "str".


---

