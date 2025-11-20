# core/math.h

## `static inline bool is_power_of_two(usize n) {`


Check if x is a power of two.
* @logic
1. (x != 0): 0 is not a power of two.
2. (x & (x - 1)) == 0:
Binary of 8:     1000
Binary of 7:     0111
8 & 7:           0000 (True)
Binary of 6:     0110
Binary of 5:     0101
6 & 5:           0100 (False)


---

## `static inline usize align_up(usize n, usize align) {`


Aligns 'n' up to the nearest multiple of 'align'.

@logic
Mask off the lower bits using ~(align - 1).
Adds (align - 1) beforehand to push it over the threshold.



**Example:**

```c
 * align_up(5, 4) -> (5 + 3) & ~3 -> 8 & ...11100 -> 8
 *
```


> **Note:** 'align' MUST be a power of two.


---

## `static inline usize align_down(usize n, usize align) {`


Aligns 'n' down to the nearest multiple of 'align'.

> **Note:** 'align' MUST be a power of two.


---

## `static inline bool is_aligned(usize n, usize align) {`


Checks if 'n' is aligned to 'align'.

> **Note:** 'align' MUST be a power of two.


---

## `static inline int clz64(u64 n) {`


Count leading zeros.

- **Returns**: The number of leading zeros. Returns 64 if n is 0.


---

## `static inline int ctz64(u64 n) {`


Count trailing zeros.

- **Returns**: The number of trailing zeros. Returns 64 if n is 0.


---

## `static inline int popcount64(u64 n) {`


Count set bits (population count).


---

## `static inline usize next_power_of_two(usize n) {`


Returns the smallest power of two greater than or equal to 'n'.

> **Note:** Returns 0 on overflow (e.g., result exceeds usize range).


---

## `#define min(a, b) \ ({`


Get the minimum of two values (type-safe, no side-effects).


---

## `#define max(a, b) \ ({`


Get the maximum of two values (type-safe, no side-effects).


---

## `#define clamp(val, low, high) \ ({`


Clamp a value between low and high (type-safe).


---

