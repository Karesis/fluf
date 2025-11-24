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
