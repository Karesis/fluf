#pragma once

#include <core/mem/allocer.h>
#include <core/msg.h>
#include <core/type.h>
#include <core/macros.h>
#include <std/strings/str.h>

/*
 * ==========================================================================
 * 1. Type Definition
 * ==========================================================================
 */

/**
 * @brief Dynamic String (Owned).
 *
 * A growable, mutable string buffer that owns its memory.
 *
 * Invariants:
 * 1. Always null-terminated (data[len] == '\0'), even if empty.
 * 2. Capacity >= len + 1.
 */
typedef struct String {
	char *data;
	usize len; /// length excluding null terminator
	usize cap; /// total capacity including null terminator space
	allocer_t alc;
} string_t;

/*
 * ==========================================================================
 * 2. Lifecycle API
 * ==========================================================================
 */

/**
 * @brief Initialize a string on the stack.
 * @param cap_hint Initial capacity (excluding null). 0 is allowed (lazy alloc).
 */
[[nodiscard]] bool string_init(string_t *s, allocer_t alc, usize cap_hint);

/**
 * @brief Free the string's internal buffer.
 */
void string_deinit(string_t *s);

/**
 * @brief Create a new string on the heap/arena.
 * @return Pointer to string_t, or nullptr on OOM.
 */
[[nodiscard]] string_t *string_new(allocer_t alc, usize cap_hint);

/**
 * @brief Destroy a heap-allocated string.
 */
void string_drop(string_t *s);

/**
 * @brief Create a string from a slice (Clone).
 */
[[nodiscard]] string_t *string_from_str(allocer_t alc, str_t s);

/**
 * @brief Create a string from a C-string (Clone).
 */
[[nodiscard]] string_t *string_from_cstr(allocer_t alc, const char *cstr);

/*
 * ==========================================================================
 * 3. Modification API
 * ==========================================================================
 */

/**
 * @brief Append a single character.
 */
[[nodiscard]] bool string_push(string_t *s, char c);

/**
 * @brief Append a string slice (str_t).
 * Use this for most append operations (it handles C-strings too via macros/casting).
 */
[[nodiscard]] bool string_append(string_t *s, str_t slice);

/**
 * @brief Append a C-string.
 */
[[nodiscard]] bool string_append_cstr(string_t *s, const char *cstr);

/**
 * @brief Append formatted data (printf style).
 */
[[nodiscard]] bool string_fmt(string_t *s, const char *fmt, ...);

/**
 * @brief Clear the string content (len = 0), keep capacity.
 * Ensures data[0] == '\0'.
 */
void string_clear(string_t *s);

/**
 * @brief Reserve capacity for at least `additional` bytes.
 */
[[nodiscard]] bool string_reserve(string_t *s, usize additional);

/*
 * ==========================================================================
 * 4. Accessor API
 * ==========================================================================
 */

/**
 * @brief Get the length (strlen).
 */
static inline usize string_len(const string_t *s)
{
	return s->len;
}

/**
 * @brief Check if empty.
 */
static inline bool string_is_empty(const string_t *s)
{
	return s->len == 0;
}

/**
 * @brief View as a slice (str_t).
 * @note The view is valid only until the string is modified.
 */
static inline str_t string_as_str(const string_t *s)
{
	if (unlikely(s->data == nullptr))
		return str("");
	return str_from_parts(s->data, s->len);
}

/**
 * @brief View as a C-string.
 * @note The pointer is valid only until the string is modified.
 */
static inline const char *string_cstr(const string_t *s)
{
	if (unlikely(s->data == nullptr))
		return "";
	return s->data;
}

/*
 * ==========================================================================
 * Format
 * ==========================================================================
 */

#define fstring "%.*s"
#define fmt_string(s) (int)((s).len), (s).data
