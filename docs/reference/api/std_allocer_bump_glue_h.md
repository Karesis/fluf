# std/allocer/bump/glue.h

## `allocer_t bump_to_allocer(bump_t *bump);`


"构造"一个 allocer_t 句柄

接受一个指向已初始化 bump_t 的指针，
并返回一个"胖指针" (allocer_t)，该指针
内部指向该 bump_t 实例和 bump vtable。


- **`bump`**: 一个指向 *已初始化* 的 bump_t 实例的指针。
- **Returns**: 一个 allocer_t 句柄。


---

