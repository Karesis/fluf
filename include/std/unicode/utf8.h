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

#pragma once

#include <core/type.h>
#include <std/strings/str.h>

/*
 * ==========================================================================
 * Types
 * ==========================================================================
 */

/// unicode Replacement Character ()
#define UTF8_REPLACEMENT_CHARACTER 0xFFFD

/// unicode Codepoint (32-bit, usually 21-bit effective)
typedef u32 rune_t;
#define rune(x) ((rune_t)(x))

/**
 * @brief Result of a decoding operation.
 */
typedef struct {
	rune_t value; /// the decoded codepoint (or REPLACEMENT if invalid)
	u8 len; /// the width in bytes (1-4) consumed.
} utf8_decode_result_t;

/*
 * ==========================================================================
 * Low-level Decoding (Stateless)
 * ==========================================================================
 */

/**
 * @brief Decode the first UTF-8 character from a byte buffer.
 *
 * @param ptr Pointer to the start of the utf8 sequence.
 * @param len Remaining length of the buffer.
 * @return A result containing the codepoint and its byte width.
 *
 * @note Safe and Lossy: If invalid UTF-8 is encountered, it returns
 * (0xFFFD, 1), allowing the caller to skip the bad byte and continue.
 */
[[nodiscard]] utf8_decode_result_t utf8_decode(const char *ptr, usize len);

/**
 * @brief Encode a codepoint into a byte buffer.
 *
 * @param cp The codepoint to encode.
 * @param buf Output buffer (must be at least 4 bytes).
 * @return Number of bytes written (0 if cp is invalid, e.g. > 0x10FFFF).
 */
[[nodiscard]] usize utf8_encode(rune_t cp, char *buf);

/*
 * ==========================================================================
 * Iterator (Stateful)
 * ==========================================================================
 * Replaces your old `chars_t`.
 */

typedef struct {
	str_t src; /// the source slice
	usize cursor; /// current byte offset
} utf8_iter_t;

/**
 * @brief Initialize iterator from a string slice.
 */
static inline utf8_iter_t utf8_iter_new(str_t s)
{
	return (utf8_iter_t){ .src = s, .cursor = 0 };
}

/**
 * @brief Get the next codepoint.
 * @param it Iterator pointer.
 * @param out_cp [out] The decoded codepoint.
 * @return true if a character was read, false if EOF.
 */
[[nodiscard]] bool utf8_next(utf8_iter_t *it, rune_t *out_cp);

/**
 * @brief Peek the next codepoint without advancing.
 */
[[nodiscard]] bool utf8_peek(utf8_iter_t *it, rune_t *out_cp);
