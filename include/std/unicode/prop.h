#pragma once
#include <core/type.h>
#include <std/unicode/utf8.h> /// for rune_t

/*
 * Basic ASCII Checks (Fast path)
 */
static inline bool unicode_is_ascii(rune_t c)
{
	return c <= 0x7F;
}
static inline bool unicode_is_ascii_digit(rune_t c)
{
	return c >= '0' && c <= '9';
}

/*
 * Unicode Properties (Backed by Binary Search Tables)
 */

/**
 * @brief Check if character is White_Space according to Unicode.
 */
bool unicode_is_whitespace(rune_t c);

/**
 * @brief Check if character is XID_Start.
 * Allowed as the first character of an identifier.
 */
bool unicode_is_xid_start(rune_t c);

/**
 * @brief Check if character is XID_Continue.
 * Allowed in the body of an identifier.
 */
bool unicode_is_xid_continue(rune_t c);

/**
 * @brief Check if character is Numeric.
 */
bool unicode_is_numeric(rune_t c);
