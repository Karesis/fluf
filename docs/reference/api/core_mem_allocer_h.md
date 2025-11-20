# core/mem/allocer.h

## `typedef struct AllocerVtable {`


The V-Table of the allocator.


---

## `typedef struct Allocer {`


An allocator fat pointer.

Combines the allocator state (self) and its behavior (vtable).


---

## `[[nodiscard]] static inline anyptr allocer_alloc(allocer_t allocer, layout_t layout) {`


Alloc memory using vtable.


---

## `static inline void allocer_free(allocer_t allocer, anyptr ptr, layout_t layout) {`


Free memory using vtable. 


> **Note:** If the input ptr is nullptr, this will do nothing and just return.


---

## `[[nodiscard]] static inline anyptr allocer_zalloc(allocer_t allocer, layout_t layout) {`


Zalloc memory using vtable (with fallback).


---

## `[[nodiscard]] static inline anyptr allocer_realloc(allocer_t allocer, anyptr ptr, layout_t old_layout, layout_t new_layout) {`


Realloc memory using vtable (with fallback).


---

