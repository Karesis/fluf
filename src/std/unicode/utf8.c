/*
 * Copyright 2025 Karesis
 * Licensed under the Apache License, Version 2.0.
 */

#include <std/unicode/utf8.h>

/*
 * ==========================================================================
 * Internal Helpers
 * ==========================================================================
 */

//// helper to return the replacement character error result
static inline utf8_decode_result_t _error_result(void)
{
	return (utf8_decode_result_t){ .value = UTF8_REPLACEMENT_CHARACTER,
				       .len = 1 };
}

//// check if a byte is a valid continuation byte (10xxxxxx)
static inline bool _is_continuation(u8 b)
{
	return (b & 0xC0) == 0x80;
}

/*
 * ==========================================================================
 * Decoding Logic
 * ==========================================================================
 */

utf8_decode_result_t utf8_decode(const char *ptr, usize len)
{
	if (len == 0) {
		/// EOF is not technically an error, but no data.
		/// returning 0 len indicates EOF to caller.
		return (utf8_decode_result_t){ 0, 0 };
	}

	const u8 *data = (const u8 *)ptr;
	u8 b0 = data[0];

	/// --- 1 Byte (ASCII) ---
	/// range: 0000 - 007F
	/// bit pattern: 0xxxxxxx
	if (b0 < 0x80) {
		return (utf8_decode_result_t){ (rune_t)b0, 1 };
	}

	/// --- 2 Bytes ---
	/// range: 0080 - 07FF
	/// bit pattern: 110xxxxx 10xxxxxx
	if ((b0 & 0xE0) == 0xC0) {
		if (len < 2)
			return _error_result();

		u8 b1 = data[1];
		if (!_is_continuation(b1))
			return _error_result();

		/// overlong check: b0 must be >= 0xC2 for 2-byte sequences
		/// (If b0 is C0 or C1, it could represent ASCII, which is illegal here)
		if (b0 < 0xC2)
			return _error_result();

		rune_t cp = ((rune_t)(b0 & 0x1F) << 6) | ((rune_t)(b1 & 0x3F));
		return (utf8_decode_result_t){ cp, 2 };
	}

	/// --- 3 Bytes ---
	/// range: 0800 - FFFF
	/// bit pattern: 1110xxxx 10xxxxxx 10xxxxxx
	if ((b0 & 0xF0) == 0xE0) {
		if (len < 3)
			return _error_result();

		u8 b1 = data[1];
		u8 b2 = data[2];
		if (!_is_continuation(b1) || !_is_continuation(b2))
			return _error_result();

		/// overlong check:
		/// if b0 == E0, b1 must be >= A0
		if (b0 == 0xE0 && b1 < 0xA0)
			return _error_result();

		/// surrogate check (U+D800 .. U+DFFF are forbidden in UTF-8):
		/// if b0 == ED, b1 must be < A0 (i.e., up to U+D7FF)
		if (b0 == 0xED && b1 >= 0xA0)
			return _error_result();

		rune_t cp = ((rune_t)(b0 & 0x0F) << 12) |
			    ((rune_t)(b1 & 0x3F) << 6) | ((rune_t)(b2 & 0x3F));
		return (utf8_decode_result_t){ cp, 3 };
	}

	/// --- 4 Bytes ---
	/// range: 10000 - 10FFFF
	/// bit pattern: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	if ((b0 & 0xF8) == 0xF0) {
		if (len < 4)
			return _error_result();

		u8 b1 = data[1];
		u8 b2 = data[2];
		u8 b3 = data[3];
		if (!_is_continuation(b1) || !_is_continuation(b2) ||
		    !_is_continuation(b3))
			return _error_result();

		/// overlong check:
		/// if b0 == F0, b1 must be >= 90
		if (b0 == 0xF0 && b1 < 0x90)
			return _error_result();

		/// bounds check (Max Unicode is U+10FFFF):
		/// if b0 == F4, b1 must be < 90.
		/// b0 > F4 is handled by the implicit logic (F5..FF won't match 0xF8 mask as F0?)
		/// wait, F5 is 11110101. (F5 & F8) == F0? No. F0 is 11110000.
		/// f5 is 11110101. Mask F8 is 11111000. Result is 11110000 (F0).
		/// so yes, F5, F6... match the mask 0xF0! We must check upper bound explicitly.
		if (b0 > 0xF4)
			return _error_result();
		if (b0 == 0xF4 && b1 >= 0x90)
			return _error_result();

		rune_t cp = ((rune_t)(b0 & 0x07) << 18) |
			    ((rune_t)(b1 & 0x3F) << 12) |
			    ((rune_t)(b2 & 0x3F) << 6) | ((rune_t)(b3 & 0x3F));
		return (utf8_decode_result_t){ cp, 4 };
	}

	/// --- Invalid Start Byte ---
	/// e.g., 10xxxxxx (continuation byte appearing at start)
	/// or 11111xxx (byte > F4)
	return _error_result();
}

/*
 * ==========================================================================
 * Encoding Logic
 * ==========================================================================
 */

usize utf8_encode(rune_t cp, char *buf)
{
	u8 *dst = (u8 *)buf;

	if (cp <= 0x7F) {
		dst[0] = (u8)cp;
		return 1;
	}

	if (cp <= 0x7FF) {
		dst[0] = (u8)(0xC0 | (cp >> 6));
		dst[1] = (u8)(0x80 | (cp & 0x3F));
		return 2;
	}

	if (cp <= 0xFFFF) {
		/// check for surrogates (D800 - DFFF)
		if (cp >= 0xD800 && cp <= 0xDFFF) {
			/// invalid, replace with Replacement Char
			/// recursively encode the replacement char
			return utf8_encode(UTF8_REPLACEMENT_CHARACTER, buf);
		}

		dst[0] = (u8)(0xE0 | (cp >> 12));
		dst[1] = (u8)(0x80 | ((cp >> 6) & 0x3F));
		dst[2] = (u8)(0x80 | (cp & 0x3F));
		return 3;
	}

	if (cp <= 0x10FFFF) {
		dst[0] = (u8)(0xF0 | (cp >> 18));
		dst[1] = (u8)(0x80 | ((cp >> 12) & 0x3F));
		dst[2] = (u8)(0x80 | ((cp >> 6) & 0x3F));
		dst[3] = (u8)(0x80 | (cp & 0x3F));
		return 4;
	}

	/// invalid Codepoint (too large)
	return utf8_encode(UTF8_REPLACEMENT_CHARACTER, buf);
}

/*
 * ==========================================================================
 * Iterator Logic
 * ==========================================================================
 */

bool utf8_next(utf8_iter_t *it, rune_t *out_cp)
{
	if (it->cursor >= it->src.len) {
		return false;
	}

	/// decode from current position
	utf8_decode_result_t res =
		utf8_decode(it->src.ptr + it->cursor, it->src.len - it->cursor);

	/// advance cursor
	it->cursor += res.len;

	if (out_cp) {
		*out_cp = res.value;
	}

	return true;
}

bool utf8_peek(utf8_iter_t *it, rune_t *out_cp)
{
	if (it->cursor >= it->src.len) {
		return false;
	}

	utf8_decode_result_t res =
		utf8_decode(it->src.ptr + it->cursor, it->src.len - it->cursor);

	if (out_cp) {
		*out_cp = res.value;
	}

	return true;
}
