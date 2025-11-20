#pragma once

#include <core/msg.h> /// for massert
#include <core/type.h> /// for usize, u64, bool, etc.
#include <limits.h> /// for CHAR_BIT

/*
 * ==========================================================================
 * Bitwise Operations
 * ==========================================================================
 */

/**
 * @brief Check if x is a power of two.
 * * @logic
 * 1. (x != 0): 0 is not a power of two.
 * 2. (x & (x - 1)) == 0:
 * Binary of 8:     1000
 * Binary of 7:     0111
 * 8 & 7:           0000 (True)
 * Binary of 6:     0110
 * Binary of 5:     0101
 * 6 & 5:           0100 (False)
 */
static inline bool is_power_of_two(usize n)
{
	/// logic: not 0, and only one bit is 1
	return (n > 0) && ((n & (n - 1)) == 0);
}

/**
 * @brief Aligns 'n' up to the nearest multiple of 'align'.
 *
 * @logic
 * Mask off the lower bits using ~(align - 1).
 * Adds (align - 1) beforehand to push it over the threshold.
 * 
 *
 * @example
 * align_up(5, 4) -> (5 + 3) & ~3 -> 8 & ...11100 -> 8
 *
 * @note 'align' MUST be a power of two.
 */
static inline usize align_up(usize n, usize align)
{
	massert(is_power_of_two(align), "Alignment must be a power of two");
	return (n + align - 1) & ~(align - 1);
}

/**
 * @brief Aligns 'n' down to the nearest multiple of 'align'.
 * @note 'align' MUST be a power of two.
 */
static inline usize align_down(usize n, usize align)
{
	massert(is_power_of_two(align), "Alignment must be a power of two");
	return n & ~(align - 1);
}

/**
 * @brief Checks if 'n' is aligned to 'align'.
 * @note 'align' MUST be a power of two.
 */
static inline bool is_aligned(usize n, usize align)
{
	massert(is_power_of_two(align), "Alignment must be a power of two");
	return (n & (align - 1)) == 0;
}

/*
 * ==========================================================================
 * Intrinsics Wrappers (64-bit base)
 * ==========================================================================
 */

/**
 * @brief Count leading zeros.
 * @return The number of leading zeros. Returns 64 if n is 0.
 */
static inline int clz64(u64 n)
{
	if (n == 0) {
		return 64;
	}
	return __builtin_clzll(n);
}

/**
 * @brief Count trailing zeros.
 * @return The number of trailing zeros. Returns 64 if n is 0.
 */
static inline int ctz64(u64 n)
{
	if (n == 0) {
		return 64;
	}
	return __builtin_ctzll(n);
}

/**
 * @brief Count set bits (population count).
 */
static inline int popcount64(u64 n)
{
	return __builtin_popcountll(n);
}

/*
 * ==========================================================================
 * Algorithms
 * ==========================================================================
 */

/**
 * @brief Returns the smallest power of two greater than or equal to 'n'.
 * @note Returns 0 on overflow (e.g., result exceeds usize range).
 */
static inline usize next_power_of_two(usize n)
{
	if (n <= 1) {
		return 1;
	}

	// Cast to u64 to ensure we have enough room for calculation
	u64 n_ll = (u64)n;

	// n-1 ensures that if n is already a power of 2, we return n.
	int leading_zeros = clz64(n_ll - 1);

	// Check for overflow case where n is essentially the max u64 power of two
	if (leading_zeros == 0) {
		return 0;
	}

	u64 next_pow = 1ULL << (64 - leading_zeros);

	// Check if result fits in usize (critical for 32-bit systems)
	if (sizeof(usize) < sizeof(u64) && next_pow > (usize)-1) {
		return 0; // Overflow for usize
	}

	return (usize)next_pow;
}

/*
 * ==========================================================================
 * Type-Safe Macros
 * ==========================================================================
 */

/**
 * @brief Get the minimum of two values (type-safe, no side-effects).
 */
#define min(a, b)                   \
	({                          \
		typeof(a) _a = (a); \
		typeof(b) _b = (b); \
		_a < _b ? _a : _b;  \
	})

/**
 * @brief Get the maximum of two values (type-safe, no side-effects).
 */
#define max(a, b)                   \
	({                          \
		typeof(a) _a = (a); \
		typeof(b) _b = (b); \
		_a > _b ? _a : _b;  \
	})

/**
 * @brief Clamp a value between low and high (type-safe).
 */
#define clamp(val, low, high)                       \
	({                                          \
		typeof(val) _v = (val);             \
		typeof(low) _l = (low);             \
		typeof(high) _h = (high);           \
		_v < _l ? _l : (_v > _h ? _h : _v); \
	})
