#include <core/mem/layout.h>
#include <core/msg/asrt.h>
#include <std/math/bitset.h>
#include <stddef.h>
#include <string.h>

#define BITSET_NUM_WORDS(num_bits) (((num_bits) + 63) / 64)
#define BITSET_WORD_INDEX(bit) ((bit) / 64)
#define BITSET_BIT_INDEX(bit) ((bit) % 64)
#define BITSET_WORD_MASK(bit) ((uint64_t)1 << BITSET_BIT_INDEX(bit))

bool bitset_init(bitset_t *bs, size_t num_bits, allocer_t *alc) {
  asrt_msg(bs != NULL, "Bitset pointer cannot be NULL");
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  bs->alc = alc;
  bs->num_bits = num_bits;
  if (num_bits == 0) {
    bs->num_words = 0;
    bs->words = NULL;
    return true;
  }

  bs->num_words = BITSET_NUM_WORDS(num_bits);

  layout_t layout = layout_of_array(uint64_t, bs->num_words);
  bs->words = allocer_zalloc(alc, layout);

  return bs->words != NULL;
}

bool bitset_init_all(bitset_t *bs, size_t num_bits, allocer_t *alc) {

  if (!bitset_init(bs, num_bits, alc)) {
    return false;
  }
  bitset_set_all(bs);
  return true;
}

void bitset_destroy(bitset_t *bs) {
  if (!bs)
    return;
  if (bs->words) {

    layout_t layout = layout_of_array(uint64_t, bs->num_words);
    allocer_free(bs->alc, bs->words, layout);
  }
  memset(bs, 0, sizeof(bitset_t));
}

void bitset_set(bitset_t *bs, size_t bit) {
  asrt_msg(bit < bs->num_bits, "Bitset index out of bounds");
  bs->words[BITSET_WORD_INDEX(bit)] |= BITSET_WORD_MASK(bit);
}

void bitset_clear(bitset_t *bs, size_t bit) {
  asrt_msg(bit < bs->num_bits, "Bitset index out of bounds");
  bs->words[BITSET_WORD_INDEX(bit)] &= ~BITSET_WORD_MASK(bit);
}

bool bitset_test(const bitset_t *bs, size_t bit) {
  asrt_msg(bit < bs->num_bits, "Bitset index out of bounds");
  return (bs->words[BITSET_WORD_INDEX(bit)] & BITSET_WORD_MASK(bit)) != 0;
}

void bitset_set_all(bitset_t *bs) {
  if (bs->num_words == 0)
    return;

  size_t full_words = bs->num_words;
  size_t remaining_bits = bs->num_bits % 64;

  if (remaining_bits != 0) {
    if (bs->num_words > 0) {
      full_words = bs->num_words - 1;
    }

    bs->words[bs->num_words - 1] = ((uint64_t)1 << remaining_bits) - 1;
  }

  for (size_t i = 0; i < full_words; i++) {
    bs->words[i] = (uint64_t)-1;
  }
}

void bitset_clear_all(bitset_t *bs) {
  if (bs->num_words == 0)
    return;
  memset(bs->words, 0, bs->num_words * sizeof(uint64_t));
}

bool bitset_equals(const bitset_t *bs1, const bitset_t *bs2) {
  if (bs1->num_bits != bs2->num_bits) {
    return false;
  }
  if (bs1->num_words == 0) {
    return true;
  }
  return memcmp(bs1->words, bs2->words, bs1->num_words * sizeof(uint64_t)) == 0;
}

void bitset_copy(bitset_t *dest, const bitset_t *src) {
  asrt_msg(dest->num_bits == src->num_bits, "Bitset copy size mismatch");
  if (src->num_words == 0)
    return;
  memcpy(dest->words, src->words, src->num_words * sizeof(uint64_t));
}

void bitset_intersect(bitset_t *dest, const bitset_t *src1,
                      const bitset_t *src2) {
  asrt_msg(dest->num_bits == src1->num_bits && src1->num_bits == src2->num_bits,
           "Bitset op size mismatch");
  for (size_t i = 0; i < dest->num_words; i++) {
    dest->words[i] = src1->words[i] & src2->words[i];
  }
}

void bitset_union(bitset_t *dest, const bitset_t *src1, const bitset_t *src2) {
  asrt_msg(dest->num_bits == src1->num_bits && src1->num_bits == src2->num_bits,
           "Bitset op size mismatch");
  for (size_t i = 0; i < dest->num_words; i++) {
    dest->words[i] = src1->words[i] | src2->words[i];
  }
}

void bitset_difference(bitset_t *dest, const bitset_t *src1,
                       const bitset_t *src2) {
  asrt_msg(dest->num_bits == src1->num_bits && src1->num_bits == src2->num_bits,
           "Bitset op size mismatch");
  for (size_t i = 0; i < dest->num_words; i++) {
    dest->words[i] = src1->words[i] & (~src2->words[i]);
  }
}

size_t bitset_count(const bitset_t *bs) {

  size_t count = 0;
  if (bs->num_words == 0)
    return 0;

  size_t full_words = bs->num_words - 1;
  for (size_t i = 0; i < full_words; i++) {
    count += __builtin_popcountll(bs->words[i]);
  }

  uint64_t last_word = bs->words[bs->num_words - 1];
  size_t remaining_bits = bs->num_bits % 64;

  if (remaining_bits == 0) {

    count += __builtin_popcountll(last_word);
  } else {

    uint64_t mask = ((uint64_t)1 << remaining_bits) - 1;
    count += __builtin_popcountll(last_word & mask);
  }

  return count;
}