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
#include <core/macros.h>
#include <std/strings/string.h>
#include <std/strings/str.h>

/*
 * ==========================================================================
 * File System Operations
 * ==========================================================================
 * A unified module for File I/O and Metadata operations.
 * Integrates tightly with `string_t` (buffer) and `str_t` (view).
 */

/**
 * @brief Check if a file exists at the given path.
 * @param path C-string path.
 */
bool file_exists(const char *path);

/**
 * @brief Remove (delete) a file.
 * @return true if successful, false if error (e.g., not found, permission).
 */
bool file_remove(const char *path);

/**
 * @brief Check if path exists and is a directory.
 */
bool fs_is_dir(const char *path);

/**
 * @brief Check if path exists and is a regular file.
 */
bool fs_is_file(const char *path);

/*
 * ==========================================================================
 * Read / Write Operations
 * ==========================================================================
 */

/**
 * @brief Read the ENTIRE contents of a file into a string builder.
 *
 * @param path Path to the file.
 * @param out  Pointer to an initialized string_t.
 * The file content will be APPENDED to this string.
 * (Use string_clear(out) beforehand if you want to overwrite).
 *
 * @return true on success, false on failure.
 *
 * @note 
 * 1. Opens file in BINARY mode ("rb").
 * 2. Intelligently pre-allocates memory to minimize reallocations.
 * 3. Handles the null-terminator invariant of string_t automatically.
 */
[[nodiscard]] bool file_read_to_string(const char *path, string_t *out);

/**
 * @brief Write a string slice to a file (Overwrite).
 *
 * @param path Path to the file. Creates it if missing, truncates if exists.
 * @param content The data to write.
 * @return true on success.
 */
[[nodiscard]] bool file_write(const char *path, str_t content);

/**
 * @brief Append a string slice to the end of a file.
 *
 * @param path Path to the file. Creates it if missing.
 * @param content The data to append.
 * @return true on success.
 */
[[nodiscard]] bool file_append(const char *path, str_t content);
