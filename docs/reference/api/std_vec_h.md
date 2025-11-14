# std/vec.h

## `typedef struct vec {`


(Opaque) 指针动态数组 (Vec<void*>)

这是一个专门用于存储 `void*` 指针的动态数组。
它不拥有它所指向的内存 (释放内存是 allocer 的责任)。

(遵循 bump_t/strintern_t 模式, 在栈上初始化)


---

## `bool vec_init(vec_t *vec, allocer_t *alc, size_t initial_capacity);`


初始化一个 vector (例如在栈上)。


- **`vec`**: 指向要初始化的 vector 实例的指针。
- **`alc`**: 用于所有内部存储的分配器 (vtable 句柄)。
- **Returns**: true (成功) 或 false (OOM)。


---

## `void vec_destroy(vec_t *vec);`


销毁 vector 的内部数据 (data 数组)。


> **Note:** 这 *不会* 释放 vec 结构体本身。
它*不会*释放存储在 vector 中的 `void*` 指针。


---

## `bool vec_push(vec_t *vec, void *ptr);`


将一个指针追加到 vector 末尾。


- **`vec`**: vector 实例。
- **`ptr`**: 要追加的 `void*` 指针。
- **Returns**: true (成功) 或 false (OOM / 扩容失败)。


---

## `void *vec_pop(vec_t *vec);`


移除并返回 vector 末尾的指针。


- **`vec`**: vector 实例。
- **Returns**: 被移除的 `void*` 指针，如果 vector 为空则返回 NULL。


---

## `void *vec_get(const vec_t *vec, size_t index);`


获取指定索引处的指针。


- **`vec`**: vector 实例。
- **`index`**: 索引。
- **Returns**: `void*` 指针，如果索引越界则返回 NULL。


---

## `void vec_set(vec_t *vec, size_t index, void *ptr);`


设置指定索引处的指针。


- **`vec`**: vector 实例。
- **`index`**: 索引。
- **`ptr`**: 要设置的 `void*` 指针。

> **Note:** 如果索引越界, 将触发 `asrt` 失败。


---

## `void vec_clear(vec_t *vec);`


清空 vector (count=0) 但保留容量。


---

## `size_t vec_count(const vec_t *vec);`


获取 vector 中的元素数量。


---

## `void **vec_data(const vec_t *vec);`


获取指向底层 `void**` 数组的指针。
*警告*: 此指针在 `vec_push` 后可能会失效。


---

