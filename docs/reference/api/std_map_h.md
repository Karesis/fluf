# std/map.h

## `u64 (*hash)(const void *key);`

Hash function: returns a 64-bit hash for the key. 

---

## `bool (*equals)(const void *key_lhs, const void *key_rhs);`

Equality function: returns true if two keys are equal. 

---

## `extern const map_ops_t MAP_OPS_U32;`


Ops for specific integer types.
Choose based on the sizeof(Key).


---

## `extern const map_ops_t MAP_OPS_PTR;`


Ops for pointers (hashes the address itself).


---

## `extern const map_ops_t MAP_OPS_CSTR;`


Ops for C-strings (const char*). Hashes the content.


---

## `#define map_put(m, key, val) \ ({`


Insert or update a value.

- **`key`**: Passed by value (will be copied into map).
- **`val`**: Passed by value.
- **Returns**: true on success, false on OOM.


---

## `#define map_get(m, key) \ ({`


Get a pointer to the value.

- **Returns**: Pointer to value, or nullptr if not found.
Allows modification: *ptr = new_val;


---

## `#define map_remove(m, key) \ ({`


Remove a key.

- **Returns**: true if key existed and was removed.


---

## `#define map_clear(m) _map_clear_impl((anyptr) & (m)) /* --- Utilities --- */ #define map_len(m) ((m).len) #define map_cap(m) ((m).cap) /* * ========================================================================== * 4. Internals * ========================================================================== */ [[nodiscard]] bool _map_init_impl(anyptr map, allocer_t alc, map_ops_t ops, usize k_sz, usize v_sz);`


Clear all entries (keeps capacity).


---

