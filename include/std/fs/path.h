#pragma once

#include <core/type.h>
#include <core/macros.h>
#include <std/strings/str.h>
#include <std/strings/string.h>

/*
 * ==========================================================================
 * Path Constants
 * ==========================================================================
 */

#if defined(_WIN32)
#define PATH_SEP '\\'
#define PATH_SEP_STR "\\"
#else
#define PATH_SEP '/'
#define PATH_SEP_STR "/"
#endif

/*
 * ==========================================================================
 * Path Query (Zero Copy / View)
 * ==========================================================================
 */

/**
 * @brief Check if a character is a path separator.
 * Handles both '/' and '\\' on Windows for robustness.
 */
static inline bool path_is_sep(char c)
{
#if defined(_WIN32)
	return c == '/' || c == '\\';
#else
	return c == '/';
#endif
}

/**
 * @brief Get the file extension (without the dot).
 *
 * Examples:
 * - "file.txt" -> "txt"
 * - "archive.tar.gz" -> "gz"
 * - "Makefile" -> "" (empty)
 * - ".gitignore" -> "gitignore" (Here we return "gitignore")
 * - "dir.with.dot/file" -> ""
 */
str_t path_ext(str_t path);

/**
 * @brief Get the file name (basename).
 *
 * Examples:
 * - "/usr/bin/gcc" -> "gcc"
 * - "src/main.c" -> "main.c"
 */
str_t path_file_name(str_t path);

/**
 * @brief Get the directory part (dirname).
 *
 * Examples:
 * - "/usr/bin/gcc" -> "/usr/bin"
 * - "file.txt" -> "" (or "." depending on philosophy. We return "" for pure relative)
 */
str_t path_dir_name(str_t path);

/*
 * ==========================================================================
 * Path Manipulation (Builder)
 * ==========================================================================
 */

/**
 * @brief Push a component to a path buffer.
 * Automatically handles separator insertion/deduplication.
 *
 * @param buf  The path string builder (modified).
 * @param component The segment to append.
 * @return true on success.
 *
 * Logic:
 * "dir" + "file" -> "dir/file"
 * "dir/" + "file" -> "dir/file" (No double slash)
 * "" + "file" -> "file"
 */
[[nodiscard]] bool path_push(string_t *buf, str_t component);

/**
 * @brief Replace the extension of the path in the buffer.
 * * @param buf The path string builder.
 * @param new_ext The new extension (without dot).
 * @return true on success.
 *
 * Logic:
 * "image.png" + "jpg" -> "image.jpg"
 * "file" + "txt" -> "file.txt"
 */
[[nodiscard]] bool path_set_ext(string_t *buf, str_t new_ext);
