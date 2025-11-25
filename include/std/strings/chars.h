#pragma once
#include <core/type.h>

/*
 * ==========================================================================
 * Character Classification (ASCII Fast Path)
 * ==========================================================================
 */

static inline bool char_is_digit(char c)
{
	return c >= '0' && c <= '9';
}

static inline bool char_is_hex(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	       (c >= 'A' && c <= 'F');
}

static inline bool char_is_alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool char_is_alphanum(char c)
{
	return char_is_alpha(c) || char_is_digit(c);
}

static inline bool char_is_space(char c)
{
	/// include vertical tab and form feed for completeness with libc isspace
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
	       c == '\f';
}
