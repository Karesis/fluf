# core/mem/allocer.h

## `typedef struct allocer_vtable {`


分配器虚函数表 (The V-Table)

定义了分配器必须实现的函数"契约"。
每个函数都接收一个 `void* self` 指针，指向具体的分配器实例
(例如 bump_t* 或 system_alloc_t*)。


---

## `void *(*alloc)(void *self, layout_t layout);`

分配内存。
成功时返回一个指针，OOM 时返回 NULL。


---

## `void (*free)(void *self, void *ptr, layout_t layout);`

释放内存。
注意：对于 Arena，这通常是一个 no-op (什么也不做)。


---

## `void *(*realloc)(void *self, void *ptr, layout_t old_layout, layout_t new_layout);`

重新分配内存。
成功时返回一个新指针，OOM 时返回 NULL。


---

## `void *(*zalloc)(void *self, layout_t layout);`

分配清零的内存。
成功时返回一个指针，OOM 时返回 NULL。


---

## `struct allocer_t {`


分配器句柄 (The "Fat Pointer")

这是用户持有的对象。它将"实例数据"(self)和"实现"(vtable)捆绑在一起。


---

## `static inline void *allocer_alloc(allocer_t *alc, layout_t layout) {`


(API) 通过 vtable 分配内存


---

## `static inline void allocer_free(allocer_t *alc, void *ptr, layout_t layout) {`


(API) 通过 vtable 释放内存


---

## `static inline void *allocer_realloc(allocer_t *alc, void *ptr, layout_t old_layout, layout_t new_layout) {`


(API) 通过 vtable 重新分配内存


---

## `static inline void *allocer_zalloc(allocer_t *alc, layout_t layout) {`


(API) 通过 vtable 分配清零的内存


---

