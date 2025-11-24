#pragma once

#include <core/type.h>
#include <core/mem/allocer.h>
#include <core/msg.h>
#include <core/macros.h>

/*
 * ==========================================================================
 * 1. Type Definition
 * ==========================================================================
 * A dense bitset backed by an array of u64 words.
 */
typedef struct BitSet {
	u64 *words;
	usize num_bits;
	usize num_words;
	allocer_t alc;
} bitset_t;

/*
 * ==========================================================================
 * 2. Lifecycle API
 * ==========================================================================
 */

/**
 * @brief Initialize a bitset with all bits set to 0.
 * @param num_bits Number of bits (capacity).
 */
[[nodiscard]] bool bitset_init(bitset_t *bs, allocer_t alc, usize num_bits);

/**
 * @brief Free internal memory.
 */
void bitset_deinit(bitset_t *bs);

/**
 * @brief Create a new bitset on the heap (all 0).
 */
[[nodiscard]] bitset_t *bitset_new(allocer_t alc, usize num_bits);

/**
 * @brief Destroy a heap-allocated bitset.
 */
void bitset_drop(bitset_t *bs);

/**
 * @brief Create a clone of an existing bitset.
 */
[[nodiscard]] bitset_t *bitset_clone(const bitset_t *src);

/*
 * ==========================================================================
 * 3. Core Operations (Inlined for Speed)
 * ==========================================================================
 */

#define _BS_WORD_IDX(bit) ((bit) / 64)
#define _BS_BIT_MASK(bit) ((u64)1 << ((bit) % 64))

/**
 * @brief Set a bit to 1.
 */
static inline void bitset_set(bitset_t *bs, usize bit)
{
	/// Range check in debug mode? Or massert?
	/// For bitsets, performance is key, so massert is good.
	massert(bit < bs->num_bits, "Bitset index out of bounds");
	bs->words[_BS_WORD_IDX(bit)] |= _BS_BIT_MASK(bit);
}

/**
 * @brief Set a bit to 0.
 */
static inline void bitset_clear(bitset_t *bs, usize bit)
{
	massert(bit < bs->num_bits, "Bitset index out of bounds");
	bs->words[_BS_WORD_IDX(bit)] &= ~_BS_BIT_MASK(bit);
}

/**
 * @brief Toggle a bit.
 */
static inline void bitset_flip(bitset_t *bs, usize bit)
{
	massert(bit < bs->num_bits, "Bitset index out of bounds");
	bs->words[_BS_WORD_IDX(bit)] ^= _BS_BIT_MASK(bit);
}

/**
 * @brief Check if a bit is 1.
 */
static inline bool bitset_test(const bitset_t *bs, usize bit)
{
	massert(bit < bs->num_bits, "Bitset index out of bounds");
	return (bs->words[_BS_WORD_IDX(bit)] & _BS_BIT_MASK(bit)) != 0;
}

/**
 * @brief Assign a specific value (true/false) to a bit.
 */
static inline void bitset_assign(bitset_t *bs, usize bit, bool value)
{
	if (value)
		bitset_set(bs, bit);
	else
		bitset_clear(bs, bit);
}

/*
 * ==========================================================================
 * 4. Bulk Operations
 * ==========================================================================
 */

void bitset_set_all(bitset_t *bs);
void bitset_clear_all(bitset_t *bs);
void bitset_flip_all(bitset_t *bs);

/**
 * @brief Count number of set bits (Population Count).
 */
usize bitset_count(const bitset_t *bs);

/**
 * @brief Check if all bits are 0.
 */
bool bitset_none(const bitset_t *bs);

/**
 * @brief Check if all bits are 1.
 */
bool bitset_all(const bitset_t *bs);

/*
 * ==========================================================================
 * 5. Set Operations (Union, Intersection, etc.)
 * ==========================================================================
 * Dest = Src1 op Src2
 */

void bitset_union(bitset_t *dest, const bitset_t *src); /// dest |= src
void bitset_intersect(bitset_t *dest, const bitset_t *src); /// dest &= src
void bitset_difference(bitset_t *dest, const bitset_t *src); /// dest &= ~src
void bitset_xor(bitset_t *dest, const bitset_t *src); /// dest ^= src

bool bitset_eq(const bitset_t *a, const bitset_t *b);
bool bitset_is_subset(const bitset_t *sub, const bitset_t *super);

/*
 * ==========================================================================
 * 6. Iterator
 * ==========================================================================
 */

/**
 * @brief Bitset Iterator State.
 * Used to iterate over all set bits (1s) efficiently.
 */
typedef struct {
	const bitset_t *bs;
	usize word_idx; /// Current word index we are scanning
	u64 current_word; /// Cache of the current word (bits are cleared as we iterate)
} bitset_iter_t;

/**
 * @brief Initialize the iterator.
 */
static inline bitset_iter_t bitset_iter(const bitset_t *bs)
{
	/// Lazy initialization: we don't load the first word yet,
	/// or we can pre-load. Let's keep it simple.
	/// Point to before the first word, or set up for the first call.
	return (bitset_iter_t){
		.bs = bs,
		.word_idx = 0,
		/// Load the first word immediately to save check in next()
		.current_word = (bs->num_words > 0) ? bs->words[0] : 0
	};
}

/**
 * @brief Get the next set bit index.
 * * @param it Iterator pointer.
 * @param out_bit [out] The absolute index of the found bit.
 * @return true if a bit was found, false if iteration finished.
 * * @note Complexity is proportional to the number of set bits, not total bits.
 * It skips zeros using CPU intrinsics (ctz).
 */
bool bitset_next(bitset_iter_t *it, usize *out_bit);

/**
 * @brief Macro for convenient iteration.
 * @param idx_var Name of the size_t variable to hold the bit index.
 * @param bs_ptr Pointer to the bitset.
 */
#define bitset_foreach(idx_var, bs_ptr)                         \
	for (bitset_iter_t _it_##idx_var = bitset_iter(bs_ptr); \
	     bitset_next(&_it_##idx_var, &idx_var);)
