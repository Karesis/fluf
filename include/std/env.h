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
#include <core/mem/allocer.h>
#include <std/strings/string.h>
#include <std/vec.h>

/*
 * ==========================================================================
 * 1. Command Line Arguments (Args Iterator)
 * ==========================================================================
 */

// Define a vector of string slices (Views into the original argv)
defVec(str_t, StrSliceVec);

/**
 * @brief Command Line Arguments Iterator.
 * Wraps argc/argv into a safe, iterable stream.
 */
typedef struct Args {
	StrSliceVec items; /// Stores str_t views of argv
	usize cursor; /// Current position
	allocer_t alc; /// Allocator for the vector
} args_t;

/**
 * @brief Parse argc/argv into an args object.
 *
 * @param out Pointer to uninitialized args_t.
 * @param alc Allocator for the internal vector.
 * @param argc From main().
 * @param argv From main().
 */
[[nodiscard]] bool args_init(args_t *out, allocer_t alc, int argc, char **argv);

/**
 * @brief Cleanup the args object.
 */
void args_deinit(args_t *args);

/**
 * @brief Declare args parser with RAII lifecycle.
 */
#define args_let(var_name, allocator, argc, argv)              \
	defer(args_deinit) args_t var_name = { 0 };            \
	massert(args_init(&(var_name), allocator, argc, argv), \
		"Args init failed")

/**
 * @brief Get the next argument (consumes it).
 * @return The argument slice, or empty string/null-ptr slice if exhausted.
 * Use `str_is_valid(s.ptr)` check if needed, or just check logic flow.
 * (Actually better: return pointer to str_t inside vec, or value).
 * Let's return value. If exhausted, returns str(""). Check `args_done()` first.
 */
str_t args_next(args_t *args);

/**
 * @brief Peek at the next argument without consuming it.
 */
str_t args_peek(args_t *args);

/**
 * @brief Check if there are more arguments.
 */
bool args_has_next(const args_t *args);

/**
 * @brief Get the remaining number of arguments.
 */
usize args_remaining(const args_t *args);

/**
 * @brief Get the program name (argv[0]).
 */
str_t args_program_name(const args_t *args);

/*
 * ==========================================================================
 * 2. Iterators (Macros)
 * ==========================================================================
 */

/**
 * @brief Iterate over remaining arguments.
 *
 * @param var      The name of the str_t variable to hold the argument.
 * @param args_ptr Pointer to the args_t object.
 *
 * @note This loop advances the cursor. It iterates from the CURRENT position.
 * Usually you want to call `args_next` once to skip the program name (argv[0])
 * before using this loop.
 *
 * @example
 * args_next(&args); // Skip argv[0]
 * args_foreach(arg, &args) {
 * printf("Arg: " fmt(str) "\n", arg);
 * }
 */
#define args_foreach(var, args_ptr)                                 \
	for (str_t var; args_has_next(args_ptr) ?                   \
				(var = args_next(args_ptr), true) : \
				false;)

/*
 * ==========================================================================
 * 3. Environment Variables
 * ==========================================================================
 */

/**
 * @brief Get an environment variable.
 *
 * @param key The env var name (e.g., "HOME", "PATH").
 * @param out Target string builder. The value will be APPENDED to it.
 * @return true if found, false if not found or error.
 *
 * @note Why append to string_t?
 * 1. Env vars can be long.
 * 2. `getenv` return value lifetime is platform-dependent/unsafe.
 * 3. We want an OWNED copy for safety.
 */
[[nodiscard]] bool env_get(const char *key, string_t *out);

/**
 * @brief Set an environment variable.
 * @return true on success.
 */
[[nodiscard]] bool env_set(const char *key, const char *value);

/**
 * @brief Unset (remove) an environment variable.
 */
bool env_unset(const char *key);

/*
 * ==========================================================================
 * 3. Process / Path Info
 * ==========================================================================
 */

/**
 * @brief Get the Current Working Directory.
 * @param out Target string builder.
 * @return true on success.
 */
[[nodiscard]] bool env_current_dir(string_t *out);
