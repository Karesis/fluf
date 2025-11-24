# std/math/bitset.h

## `[[nodiscard]] bool bitset_init(bitset_t *bs, allocer_t alc, usize num_bits);`


Initialize a bitset with all bits set to 0.

- **`num_bits`**: Number of bits (capacity).


---

## `void bitset_deinit(bitset_t *bs);`


Free internal memory.


---

## `[[nodiscard]] bitset_t *bitset_new(allocer_t alc, usize num_bits);`


Create a new bitset on the heap (all 0).


---

## `void bitset_drop(bitset_t *bs);`


Destroy a heap-allocated bitset.


---

## `[[nodiscard]] bitset_t *bitset_clone(const bitset_t *src);`


Create a clone of an existing bitset.


---

## `static inline void bitset_set(bitset_t *bs, usize bit) {`


Set a bit to 1.


---

## `static inline void bitset_clear(bitset_t *bs, usize bit) {`


Set a bit to 0.


---

## `static inline void bitset_flip(bitset_t *bs, usize bit) {`


Toggle a bit.


---

## `static inline bool bitset_test(const bitset_t *bs, usize bit) {`


Check if a bit is 1.


---

## `static inline void bitset_assign(bitset_t *bs, usize bit, bool value) {`


Assign a specific value (true/false) to a bit.


---

## `usize bitset_count(const bitset_t *bs);`


Count number of set bits (Population Count).


---

## `bool bitset_none(const bitset_t *bs);`


Check if all bits are 0.


---

## `bool bitset_all(const bitset_t *bs);`


Check if all bits are 1.


---

## `typedef struct {`


Bitset Iterator State.
Used to iterate over all set bits (1s) efficiently.


---

## `static inline bitset_iter_t bitset_iter(const bitset_t *bs) {`


Initialize the iterator.


---

## `bool bitset_next(bitset_iter_t *it, usize *out_bit);`


Get the next set bit index.
* @param it Iterator pointer.

- **`out_bit`**: [out] The absolute index of the found bit.
- **Returns**: true if a bit was found, false if iteration finished.
* @note Complexity is proportional to the number of set bits, not total bits.
It skips zeros using CPU intrinsics (ctz).


---

## `#define bitset_foreach(idx_var, bs_ptr) \ for (bitset_iter_t _it_##idx_var = bitset_iter(bs_ptr);`


Macro for convenient iteration.

- **`idx_var`**: Name of the size_t variable to hold the bit index.
- **`bs_ptr`**: Pointer to the bitset.


---

