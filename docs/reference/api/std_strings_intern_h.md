# std/strings/intern.h

## `typedef struct Symbol {`


A unique identifier for an interned string.
Using a specific type prevents accidental arithmetic or confusion with other ints.


---

## `static inline bool sym_eq(symbol_t a, symbol_t b) {`


Helper to compare symbols.


---

## `/// define the specific Map type: Key=str_t, Value=symbol_t /// we need this definition to embed it in the Interner struct. defMap(str_t, symbol_t, StrMap);`


Helper to check if symbol is valid (assuming 0 is valid, but maybe u32_max is invalid).
Let's assume all symbols generated are valid.


---

## `typedef struct Interner {`


String Interner.

Data structure:
1. `pool`: A bump allocator to store the actual string bytes (stable addresses).
2. `map`:  str_t -> symbol_t (For deduplication/interning).
3. `vec`:  symbol_t -> str_t (For resolution/lookup).


---

## `[[nodiscard]] bool intern_init(interner_t *it, allocer_t alc);`


Initialize the interner.

- **`alc`**: The backing allocator (used by internal Bump, Map, and Vec).


---

## `void intern_deinit(interner_t *it);`


Destroy the interner and free all memory.


---

## `[[nodiscard]] interner_t *intern_new(allocer_t alc);`


Create a new interner on the heap.


---

## `void intern_drop(interner_t *it);`


Drop a heap-allocated interner.


---

## `symbol_t intern(interner_t *it, str_t s);`


Intern a string slice.

Logic:
1. Check if `s` is already in the map. If yes, return existing symbol.
2. If no:
a. Allocate memory for `s` in `it->pool` (null-terminated).
b. Create a new symbol ID (current vec len).
c. Insert into `map`.
d. Push into `vec`.
e. Return new symbol.


- **Returns**: The symbol. (Panics on OOM currently, or we can discuss result).
Let's stick to "return symbol, panic on OOM" for simplicity in compilers,
or check validity. For now, assume success or panic inside alloc.


---

## `static inline symbol_t intern_cstr(interner_t *it, const char *s) {`


Intern a C-style string.


---

## `str_t intern_resolve(const interner_t *it, symbol_t sym);`


Resolve a symbol back to a string slice.

> **Note:** O(1) operation.


---

## `const char *intern_resolve_cstr(const interner_t *it, symbol_t sym);`


Resolve a symbol back to a C-string.

> **Note:** O(1) operation. The string is guaranteed to be null-terminated
because we allocated it that way in the pool.


---

## `usize intern_count(const interner_t *it);`


Get the number of unique interned strings.


---

