/*
 *    Copyright 2025 Karesis
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <std/math/bitset.h>
#include <core/math.h>
#include <string.h> /// memset, memcpy

/// --- Internal Helpers ---

/// mask for the last partial word
static inline u64 _last_word_mask(usize num_bits)
{
	usize rem = num_bits % 64;
	return (rem == 0) ? (u64)-1 : ((u64)1 << rem) - 1;
}

/// --- Lifecycle ---

bool bitset_init(bitset_t *bs, allocer_t alc, usize num_bits)
{
	bs->alc = alc;
	bs->num_bits = num_bits;

	if (num_bits == 0) {
		bs->num_words = 0;
		bs->words = nullptr;
		return true;
	}

	/// calculate words needed: (bits + 63) / 64
	/// use checked arithmetic if paranoid, but usize usually fits
	bs->num_words = (num_bits + 63) / 64;

	layout_t l = layout_of_array(u64, bs->num_words);
	bs->words = (u64 *)allocer_zalloc(alc, l); /// zero init is important

	return bs->words != nullptr;
}

void bitset_deinit(bitset_t *bs)
{
	if (bs->words) {
		layout_t l = layout_of_array(u64, bs->num_words);
		allocer_free(bs->alc, bs->words, l);
	}
	bs->words = nullptr;
	bs->num_bits = 0;
	bs->num_words = 0;
}

bitset_t *bitset_new(allocer_t alc, usize num_bits)
{
	layout_t l = layout_of(bitset_t);
	bitset_t *bs = (bitset_t *)allocer_alloc(alc, l);
	if (bs) {
		if (!bitset_init(bs, alc, num_bits)) {
			allocer_free(alc, bs, l);
			return nullptr;
		}
	}
	return bs;
}

void bitset_drop(bitset_t *bs)
{
	if (bs) {
		allocer_t alc = bs->alc;
		bitset_deinit(bs);
		allocer_free(alc, bs, layout_of(bitset_t));
	}
}

bitset_t *bitset_clone(const bitset_t *src)
{
	bitset_t *dst = bitset_new(src->alc, src->num_bits);
	if (dst && src->words) {
		memcpy(dst->words, src->words, src->num_words * sizeof(u64));
	}
	return dst;
}

/* --- Bulk Ops --- */

void bitset_set_all(bitset_t *bs)
{
	if (bs->num_words == 0)
		return;

	/// set all full words to -1 (all 1s)
	memset(bs->words, 0xFF, bs->num_words * sizeof(u64));

	/// clean up the last partial word (so extra bits are 0)
	/// invariant: unused bits in last word should always be 0?
	/// actually, set_all implies setting *valid* bits to 1.
	/// if we set unused bits to 1, `count` logic gets messy.
	/// so we must mask the last word.

	bs->words[bs->num_words - 1] &= _last_word_mask(bs->num_bits);
}

void bitset_clear_all(bitset_t *bs)
{
	if (bs->words) {
		memset(bs->words, 0, bs->num_words * sizeof(u64));
	}
}

void bitset_flip_all(bitset_t *bs)
{
	for (usize i = 0; i < bs->num_words; ++i) {
		bs->words[i] = ~bs->words[i];
	}
	/// mask last word to keep unused bits 0
	if (bs->num_words > 0) {
		bs->words[bs->num_words - 1] &= _last_word_mask(bs->num_bits);
	}
}

usize bitset_count(const bitset_t *bs)
{
	usize cnt = 0;
	for (usize i = 0; i < bs->num_words; ++i) {
		cnt += (usize)popcount64(bs->words[i]);
	}
	return cnt;
}

bool bitset_none(const bitset_t *bs)
{
	for (usize i = 0; i < bs->num_words; ++i) {
		if (bs->words[i] != 0)
			return false;
	}
	return true;
}

bool bitset_all(const bitset_t *bs)
{
	if (bs->num_bits == 0)
		return true; /// vacuously true

	/// check full words
	for (usize i = 0; i < bs->num_words - 1; ++i) {
		if (bs->words[i] != (u64)-1)
			return false;
	}

	/// check last partial word
	/// we need to verify that all *valid* bits are 1.
	/// the _last_word_mask gives us 1s for all valid bits.
	u64 mask = _last_word_mask(bs->num_bits);
	return bs->words[bs->num_words - 1] == mask;
}

bool bitset_is_subset(const bitset_t *sub, const bitset_t *super)
{
	massert(sub->num_bits == super->num_bits, "Bitset size mismatch");

	for (usize i = 0; i < sub->num_words; ++i) {
		/// if sub has a bit set (1) where super has (0),
		/// then (sub & ~super) will be non-zero.
		if ((sub->words[i] & ~super->words[i]) != 0) {
			return false;
		}
	}
	return true;
}

/* --- Set Ops --- */

/// helper macro for binary ops loop
#define BITSET_OP_LOOP(dest, src, op)                                         \
	massert((dest)->num_bits == (src)->num_bits, "Bitset size mismatch"); \
	for (usize i = 0; i < (dest)->num_words; ++i) {                       \
		(dest)->words[i] op(src)->words[i];                           \
	}

void bitset_union(bitset_t *dest, const bitset_t *src)
{
	BITSET_OP_LOOP(dest, src, |=);
}

void bitset_intersect(bitset_t *dest, const bitset_t *src)
{
	BITSET_OP_LOOP(dest, src, &=);
}

void bitset_xor(bitset_t *dest, const bitset_t *src)
{
	BITSET_OP_LOOP(dest, src, ^=);
}

void bitset_difference(bitset_t *dest, const bitset_t *src)
{
	massert(dest->num_bits == src->num_bits, "Bitset size mismatch");
	for (usize i = 0; i < dest->num_words; ++i) {
		dest->words[i] &= ~(src->words[i]);
	}
}

bool bitset_eq(const bitset_t *a, const bitset_t *b)
{
	if (a->num_bits != b->num_bits)
		return false;
	return memcmp(a->words, b->words, a->num_words * sizeof(u64)) == 0;
}

/* --- Iters --- */

bool bitset_next(bitset_iter_t *it, usize *out_bit)
{
	const bitset_t *bs = it->bs;

	/// loop until we find a non-zero word or run out of words
	while (it->current_word == 0) {
		it->word_idx++;
		if (it->word_idx >= bs->num_words) {
			return false; /// end of bitset
		}
		it->current_word = bs->words[it->word_idx];
	}

	/// found a word with at least one '1' bit

	/// 1. find the lowest set bit (number of trailing zeros)
	int bit_in_word = ctz64(it->current_word);

	/// 2. calculate absolute index
	*out_bit = (it->word_idx * 64) + bit_in_word;

	/// 3. clear this bit from our cached word so we don't find it again
	///    x & (x - 1) clears the lowest set bit
	it->current_word &= (it->current_word - 1);

	return true;
}
