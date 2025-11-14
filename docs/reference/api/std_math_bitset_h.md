# std/math/bitset.h

## `typedef struct bitset {`


一个高密度位集 (在栈上初始化)


---

## `bool bitset_init(bitset_t *bs, size_t num_bits, allocer_t *alc);`


初始化一个新的、所有位都为 0 的位集


- **`bs`**: 指向要初始化的 bitset_t 实例 (例如在栈上)
- **`num_bits`**: 集合中所需的位数
- **`alc`**: 用于分配内部 words 数组的分配器
- **Returns**: true (成功) 或 false (OOM)


---

## `bool bitset_init_all(bitset_t *bs, size_t num_bits, allocer_t *alc);`


初始化一个新的、所有位都为 1 的位集 (全集)


---

## `void bitset_destroy(bitset_t *bs);`


销毁位集的内部存储 (words 数组)

> **Note:** 这不会释放 bitset_t 结构体本身


---

## `size_t bitset_count(const bitset_t *bs);`


[调试用] 统计集合中 1 的数量 (使用 Clang intrinsic，非常快)


---

