#pragma once

/// for DO_NOTHING, is_pointer, is_struct
#include <core/macros.h>

#include <stdio.h> /// for fprintf, stderr
#include <stdlib.h> /// for abort
/// for va_list
#include <stdarg.h>

/// Log levels
#define _LOG_LEVEL_DEBUG "DEBUG"
#define _LOG_LEVEL_INFO "INFO"
#define _LOG_LEVEL_WARN "WARN"
#define _LOG_LEVEL_ERROR "ERROR"
#define _LOG_LEVEL_PANIC "PANIC"

/**
 * @brief Log an info message.
 */
#define log_info(fmt, ...)                                      \
	do {                                                    \
		fprintf(stderr, "[%s] [%s:%d] %s(): " fmt "\n", \
			_LOG_LEVEL_INFO, __FILE__, __LINE__,    \
			__func__ __VA_OPT__(, ) __VA_ARGS__);   \
	} while (0)

/**
 * @brief Log a warning message.
 */
#define log_warn(fmt, ...)                                      \
	do {                                                    \
		fprintf(stderr, "[%s] [%s:%d] %s(): " fmt "\n", \
			_LOG_LEVEL_WARN, __FILE__, __LINE__,    \
			__func__ __VA_OPT__(, ) __VA_ARGS__);   \
	} while (0)

/**
 * @brief Log an error message.
 */
#define log_error(fmt, ...)                                     \
	do {                                                    \
		fprintf(stderr, "[%s] [%s:%d] %s(): " fmt "\n", \
			_LOG_LEVEL_ERROR, __FILE__, __LINE__,   \
			__func__ __VA_OPT__(, ) __VA_ARGS__);   \
	} while (0)

/**
 * @brief Log a panic message and abort.
 *
 * @note This is for unconditional panic.
 * @note This macro NEVER returns.
 */
#define log_panic(fmt, ...)                                     \
	do {                                                    \
		fprintf(stderr, "[%s] [%s:%d] %s(): " fmt "\n", \
			_LOG_LEVEL_PANIC, __FILE__, __LINE__,   \
			__func__ __VA_OPT__(, ) __VA_ARGS__);   \
		abort();                                        \
	} while (0)

/**
 * @brief Mark a piece of code as "Not Yet Implemented".
 *
 * @note This macro will ALWAYS panic and abort, in both
 * Debug and Release builds, to prevent shipping
 * unfinished code.
 */
#define todo(fmt, ...)                                                        \
	do {                                                                  \
		fprintf(stderr,                                               \
			"[%s] [%s:%d] %s(): [TODO] Not yet implemented: " fmt \
			"\n",                                                 \
			_LOG_LEVEL_PANIC, __FILE__, __LINE__,             \
			__func__ __VA_OPT__(, ) __VA_ARGS__);                 \
		abort();                                                      \
	} while (0)

#ifdef NDEBUG

// While release
#define dbg(fmt, ...) DO_NOTHING
#define dump(ptr) DONOTHING
#define asserrt(cond) DO_NOTHING
#define massert(cond, fmt, ...) DO_NOTHING
#define _unreachable_impl() __builtin_unreachable()

#else

/**
 * @brief Print debug messages.
 *
 * @note [DEBUG] [file:line] func(): <message>
 */
#define dbg(fmt, ...)                                           \
	do {                                                    \
		fprintf(stderr, "[%s] [%s:%d] %s(): " fmt "\n", \
			_LOG_LEVEL_DEBUG, __FILE__, __LINE__,   \
			__func__ __VA_OPT__(, ) __VA_ARGS__);   \
	} while (0)

/// inner used by dump
static inline int _stderr_printer(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = vfprintf(stderr, fmt, args);
	va_end(args);
	return ret;
}

/**
 * @brief Recursively dump a struct's contents to stderr for debugging.
 *
 * This macro acts like a "pretty-printer" for C structures. It utilizes
 * Clang's `__builtin_dump_struct` to inspect and print all fields of
 * the target struct automatically.
 *
 * @param ptr A POINTER to the struct instance (e.g., `&my_var`).
 * Passing a struct by value will trigger a compile-time error.
 *
 * @note **Compile-time Safety**: This macro uses `static_assert` to ensure:
 * 1. The argument is strictly a pointer.
 * 2. The pointed-to type is a struct or union.
 *
 * @note **Zero Overhead**: In Release builds (`NDEBUG` defined), this
 * macro expands to `DO_NOTHING` and incurs no runtime cost.
 *
 * @example
 * struct Rect r = { .x = 10, .y = 20 };
 * dump(&r);      /// Correct: output to stderr with file/line info
 * /// dump(r);   /// Compile Error: "Argument must be a POINTER"
 */
#define dump(ptr)                                                                 \
	do {                                                                      \
		fprintf(stderr, "[%s] [%s:%d] %s(): Dump Struct " #ptr ":\n",     \
			_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__);          \
                                                                                  \
		static_assert(                                                    \
			is_pointer(ptr),                                          \
			"pstruct error: Argument must be a POINTER (e.g. &obj)"); \
                                                                                  \
		static_assert(                                                    \
			is_struct(*(ptr)),                                        \
			"pstruct error: Pointer must point to a STRUCT");         \
                                                                                  \
		__builtin_dump_struct(ptr, _stderr_printer);                      \
	} while (0)

/**
 * @brief Simple assertion.
 *
 * @note [PANIC] [file:line] func(): Assertion Failed: (cond)
 * @note This macro aborts if condition is false.
 */
#define asserrt(cond)                                                          \
	do {                                                                   \
		if (!(cond)) {                                                 \
			fprintf(stderr,                                        \
				"[%s] [%s:%d] %s(): Assertion Failed: (%s)\n", \
				_LOG_LEVEL_PANIC, __FILE__, __LINE__,          \
				__func__, #cond);                              \
			abort();                                               \
		}                                                              \
	} while (0)

/**
 * @brief Assertion with a message.
 *
 * @note [PANIC] [file:line] func(): Assertion Failed: (cond)
 * @note   Message: <message>
 * @note This macro aborts if condition is false.
 */
#define massert(cond, fmt, ...)                                               \
	do {                                                                  \
		if (!(cond)) {                                                \
			fprintf(stderr,                                       \
				"[%s] [%s:%d] %s(): Assertion Failed: (%s)\n" \
				"  Message: " fmt "\n",                       \
				_LOG_LEVEL_PANIC, __FILE__, __LINE__,         \
				__func__, #cond __VA_OPT__(, ) __VA_ARGS__);  \
			abort();                                              \
		}                                                             \
	} while (0)

/**
 * @brief Print message and panic.
 *
 *     This macro is used by unreach() and 
 *     is not supposed to used as dirictly.
 *
 * @note [PANIC] [file:line] func(): Reached UNREACHABLE code path
 */
#define _unreachable_impl()                                                   \
	do {                                                                  \
		fprintf(stderr,                                               \
			"[%s] [%s:%d] %s(): Reached UNREACHABLE code path\n", \
			_LOG_LEVEL_PANIC, __FILE__, __LINE__, __func__);      \
		abort();                                                      \
	} while (0)

#endif

/**
 * @brief Mark a code path as unreachable.
 *
 * @note In Debug builds, this will panic and abort.
 * @note In Release builds, this serves as an optimization hint
 * to the compiler (e.g., __builtin_unreachable()).
 */
#define unreach() _unreachable_impl()
