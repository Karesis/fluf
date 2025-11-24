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
 * 2. Environment Variables
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
