# std/hashmap/strhashmap.h

## `typedef struct strhashmap strhashmap_t;`


(Opaque) 字符串哈希表

这是一个 "const char* -> void*" 的哈希表。
它使用开放寻址和线性探测。

- 键 (const char*) 在插入时会被 *复制* 到分配器中。
- 值 (void*) 按原样存储。
- 它不拥有 `void*` 值指向的内存。


---

## `strhashmap_t *strhashmap_new(allocer_t *alc, size_t initial_capacity);`


创建一个新的字符串哈希表。


- **`alc`**: 用于所有内部存储的分配器 (vtable 句柄)
- **`initial_capacity`**: 初始容量 (槽位数)。如果为 0，将使用默认值。
- **Returns**: 一个新的哈希表，或 OOM 时返回 NULL。


---

## `void strhashmap_free(strhashmap_t *map);`


释放哈希表及其所有内部存储。


> **Note:** 这 *不会* 释放存储在内的 `void*` 值。
它会释放哈希表为 *键* (keys) 复制的字符串。


---

## `bool strhashmap_put(strhashmap_t *map, const char *key, void *value);`


插入或更新一个键值对。

`key` 字符串会被*复制*到哈希表的内部存储中 (使用分配器)。
如果键已存在，旧的 `void*` 值将被替换 (不释放)。


- **`map`**: 哈希表
- **`key`**: 要插入的 C 字符串键。
- **`value`**: 要关联的 `void*` 指针。
- **Returns**: true (成功) 或 false (OOM)。


---

## `void *strhashmap_get(strhashmap_t *map, const char *key);`


获取一个键关联的值。

- **`map`**: 哈希表
- **`key`**: 要查找的 C 字符串键。
- **Returns**: 关联的 `void*` 指针，如果未找到则返回 NULL。

> **Note:** 如果 `NULL` 是一个有效值，请使用 `strhashmap_get_ptr`。


---

## `bool strhashmap_get_ptr(strhashmap_t *map, const char *key, void **out_value);`


(更安全的 Get) 检查键是否存在并获取值。


- **`map`**: 哈希表
- **`key`**: 要查找的 C 字符串键。
- **`out_value`**: [out] 如果找到，值的指针将被写入这里。
- **Returns**: true (如果找到) 或 false (如果未找到)。


---

## `bool strhashmap_delete(strhashmap_t *map, const char *key);`


从哈希表中删除一个键。


- **`map`**: 哈希表
- **`key`**: 要删除的 C 字符串键。
- **Returns**: true (如果删除成功) 或 false (如果未找到)。


---

## `void strhashmap_clear(strhashmap_t *map);`


清除哈希表中的所有条目，但不释放其容量。
* 这允许为下一次编译重用符号表，而无需重新分配。


---

## `size_t strhashmap_count(const strhashmap_t *map);`


获取哈希表中的条目数。


---

