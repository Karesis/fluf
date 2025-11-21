#pragma once

#include <core/type.h>
#include <core/msg.h>
#include <core/macros.h>
#include <string.h>

/*
 * ==========================================================================
 * 1. Type Definition
 * ==========================================================================
 */

/**
 * @brief String Slice (String View).
 *
 * A non-owning "fat pointer" representing a string that is NOT necessarily
 * null-terminated.
 *
 * Advantages:
 * 1. O(1) length retrieval.
 * 2. Cheap to copy (just two integers).
 * 3. Can reference substrings without memory allocation.
 */
typedef struct Str {
    const char *ptr;
    usize len;
} str_t;

/*
 * ==========================================================================
 * 2. Constructors
 * ==========================================================================
 */

/**
 * @brief Create a slice from a string literal.
 *
 * @note sizeof("lit") includes the null terminator, so we subtract 1.
 */
#define str(literal) ((str_t){ .ptr = ("" literal), .len = sizeof(literal) - 1 })

/**
 * @brief Create a slice from a C-style string (const char*).
 * @note O(n) operation (calls strlen).
 */
static inline str_t str_from_cstr(const char *cstr)
{
    if (unlikely(cstr == nullptr)) {
        return (str_t){ .ptr = nullptr, .len = 0 };
    }
    return (str_t){ .ptr = cstr, .len = strlen(cstr) };
}

/**
 * @brief Create a slice from pointer and length.
 *
 * @note This function trusts the caller!
 * It assumes `ptr` points to at least `len` readable bytes.
 * C cannot verify this at runtime without heavy tooling (like ASan).
 */
static inline str_t str_from_parts(const char *ptr, usize len)
{
    /// Safety Check: If len > 0, ptr must not be NULL.
    if (len > 0) {
        massert(ptr != nullptr, "str_from_parts: ptr is NULL but len is %zu", len);
    }
    
    return (str_t){ .ptr = ptr, .len = len };
}

/*
 * ==========================================================================
 * 3. Comparison & Inspection
 * ==========================================================================
 */

/**
 * @brief Check if the slice is empty.
 */
static inline bool str_is_empty(str_t s)
{
    return s.len == 0;
}

/**
 * @brief Compare two slices for equality.
 */
static inline bool str_eq(str_t a, str_t b)
{
    if (a.len != b.len) {
        return false;
    }
    if (a.len == 0) {
        return true;
    }
    return memcmp(a.ptr, b.ptr, a.len) == 0;
}

/**
 * @brief Compare slice with a C-string.
 */
static inline bool str_eq_cstr(str_t a, const char *b_cstr)
{
    usize b_len = strlen(b_cstr);
    if (a.len != b_len) {
        return false;
    }
    return memcmp(a.ptr, b_cstr, a.len) == 0;
}

/**
 * @brief Check if slice starts with a prefix.
 */
static inline bool str_starts_with(str_t s, str_t prefix)
{
    if (s.len < prefix.len) {
        return false;
    }
    return memcmp(s.ptr, prefix.ptr, prefix.len) == 0;
}

/**
 * @brief Check if slice ends with a suffix.
 */
static inline bool str_ends_with(str_t s, str_t suffix)
{
    if (s.len < suffix.len) {
        return false;
    }
    return memcmp(s.ptr + (s.len - suffix.len), suffix.ptr, suffix.len) == 0;
}

/*
 * ==========================================================================
 * 4. Manipulation (Trim / Split)
 * ==========================================================================
 */

static inline bool _str_is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/**
 * @brief Trim whitespace from the start.
 */
static inline str_t str_trim_left(str_t s)
{
    usize start = 0;
    while (start < s.len && _str_is_whitespace(s.ptr[start])) {
        start++;
    }
    return (str_t){ .ptr = s.ptr + start, .len = s.len - start };
}

/**
 * @brief Trim whitespace from the end.
 */
static inline str_t str_trim_right(str_t s)
{
    usize end = s.len;
    while (end > 0 && _str_is_whitespace(s.ptr[end - 1])) {
        end--;
    }
    return (str_t){ .ptr = s.ptr, .len = end };
}

/**
 * @brief Trim whitespace from both ends.
 */
static inline str_t str_trim(str_t s)
{
    return str_trim_right(str_trim_left(s));
}

/**
 * @brief Iterator: Split slice by a delimiter character.
 *
 * @param input [in/out] The slice to split. Will be updated to point to the remainder.
 * @param delim The delimiter character.
 * @param out_chunk [out] The extracted substring.
 * @return true if a chunk was found, false if iteration is finished.
 *
 * @example
 * str_t s = str("a,b");
 * str_t chunk;
 * while (str_split(&s, ',', &chunk)) { ... }
 */
static inline bool str_split(str_t *input, char delim, str_t *out_chunk)
{
    /// check if iteration finished (sentinel logic: ptr is null)
    if (input->ptr == nullptr) {
        return false;
    }

    /// check empty input case
    if (input->len == 0) {
        *out_chunk = (str_t){ .ptr = nullptr, .len = 0 }; // empty chunk
        input->ptr = nullptr; // mark as finished next time
        return true; // emit one last empty token if logically needed, or adjust logic
    }

    const void *found = memchr(input->ptr, delim, input->len);

    if (found) {
        usize dist = (const char *)found - input->ptr;
        *out_chunk = (str_t){ .ptr = input->ptr, .len = dist };
        
        /// advance input past the delimiter
        input->ptr += dist + 1;
        input->len -= dist + 1;
    } else {
        /// last chunk
        *out_chunk = *input;
        /// mark as finished
        input->ptr = nullptr;
        input->len = 0;
    }

    return true;
}

/**
 * @brief Iterator: Split slice by lines (handles \n and \r\n).
 *
 * Logic differences from str_split:
 * 1. It treats '\r\n' as a single newline.
 * 2. It does NOT emit an empty string if the input ends with a newline.
 * 3. It returns false immediately if input is empty.
 *
 * @return true if a line was read, false if EOF.
 */
static inline bool str_split_line(str_t *input, str_t *out_line)
{
    /// 1. Check EOF / Empty
    /// Unlike str_split, we stop immediately if len is 0.
    /// This prevents the "trailing empty line" issue.
    if (input->len == 0) {
        return false;
    }

    /// 2. Find next newline
    const void *nl = memchr(input->ptr, '\n', input->len);

    if (nl) {
        /// Found a newline
        usize dist = (const char *)nl - input->ptr;
        *out_line = (str_t){ .ptr = input->ptr, .len = dist };

        /// Advanced: Handle Windows "\r\n"
        /// If the char before \n is \r, shrink the output length by 1.
        if (dist > 0 && out_line->ptr[dist - 1] == '\r') {
            out_line->len--;
        }

        /// Advance input past "\n"
        input->ptr += dist + 1;
        input->len -= dist + 1;
    } else {
        /// No newline found, this is the last line (rest of the string)
        *out_line = *input;

        /// Consume everything
        input->ptr += input->len; // Point to end (or null) is fine logic here
        input->len = 0;
    }

    return true;
}

/*
 * ==========================================================================
 * 5. Iterators (Macros)
 * ==========================================================================
 */

/**
 * @brief Iterate over a slice by splitting it.
 *
 * @param var   The name of the loop variable (type: str_t).
 * This variable is declared automatically within the loop scope.
 * @param src   The source slice to iterate over (str_t).
 * Note: The source is NOT modified (a copy is used for iteration).
 * @param delim The delimiter character (char).
 *
 * @example
 * str_t s = str("apple,banana,orange");
 * str_for_split(fruit, s, ',') {
 * // fruit is str_t
 * printf("Item: " fmt(str) "\n", fruit);
 * }
 */
#define str_for_split(var, src, delim)                  \
    for (str_t _state_##var = (src), var;               \
         str_split(&_state_##var, (delim), &var);       \
        )

/**
 * @brief Iterate over lines in a slice.
 * Handles Unix (\n) and Windows (\r\n) line endings automatically.
 */
#define str_for_lines(line, src) \
    for (str_t _state_##line = (src), line; \
         str_split_line(&_state_##line, &line); \
        )

/**
 * @brief Iterate over bytes (chars) in a slice.
 *
 * @param var The name of the loop variable (type: char).
 * @param src The source slice.
 *
 * Logic:
 * 1. Outer loop manages index `_i`.
 * 2. Inner loop declares `var` AND a flag `_once`.
 * 3. Inner loop runs exactly once because `_once` is set to 0 after first run.
 */
#define str_for_each(var, src)                                      \
    for (usize _i_##var = 0; _i_##var < (src).len; ++_i_##var)      \
        for (char var = (src).ptr[_i_##var], _once_##var = 1;       \
             _once_##var;                                           \
             _once_##var = 0)
