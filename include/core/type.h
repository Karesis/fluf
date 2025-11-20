#pragma once

/// for bool, true, false
#include <stdbool.h>
/// for strcmp
#include <string.h>
/// for uintptr_t
#include <stdint.h>
/// for ssize_t
#include <sys/types.h>

/// === Explicit Type Casting ===

/// --- Unsigned Integers ---
#define u8(x) ((uint8_t)(x))
#define u16(x) ((uint16_t)(x))
#define u32(x) ((uint32_t)(x))
#define u64(x) ((uint64_t)(x))

/// --- Signed Integers ---
#define i8(x) ((int8_t)(x))
#define i16(x) ((int16_t)(x))
#define i32(x) ((int32_t)(x))
#define i64(x) ((int64_t)(x))

/// --- System Types ---

/**
 * @brief Construct a size_t (unsigned system word size).
 * Equivalent to Rust's usize.
 */
#define usize(x) ((size_t)(x))

/**
 * @brief Construct a ssize_t (signed system word size).
 * Equivalent to Rust's isize.
 */
#define isize(x) ((ssize_t)(x))

/**
 * @brief Construct a uintptr_t (unsigned pointer-sized int).
 * Used for pointer arithmetic or bitwise operations on addresses.
 */
#define uptr(x) ((uintptr_t)(x))

/// --- Floating Point ---
#define f32(x) ((float)(x))
#define f64(x) ((double)(x))

/// === Type Definitions === 

/// unsigned int
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/// signed int
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

/// size
typedef size_t   usize;
typedef ssize_t  isize;
typedef char    byte;

/// float
typedef float    f32;
typedef double   f64;

/// pointer
typedef uintptr_t uptr;
typedef void * anyptr;

/**
 * @brief Get the standard printf format specifier for a type.
 * @usage 
 * printf("Value is " fmt(x) "\n", x);
 */
#define fmt(x)                              \
	_Generic((x),                       \
		bool: "%d",                 \
		char: "%c",                 \
		signed char: "%hhd",        \
		unsigned char: "%hhu",      \
		short: "%hd",               \
		unsigned short: "%hu",      \
		int: "%d",                  \
		unsigned int: "%u",         \
		long: "%ld",                \
		unsigned long: "%lu",       \
		long long: "%lld",          \
		unsigned long long: "%llu", \
		float: "%f",                \
		double: "%f",               \
		long double: "%Lf",         \
		char *: "%s",               \
		const char *: "%s",         \
		void *: "%p",               \
		const void *: "%p",         \
		default: "%p")

/**
 * @brief Internal helper for safe string comparison.
 * Handles nullptr pointers gracefully: nullptr == nullptr, nullptr != "str".
 */
static inline bool _safe_str_eq(const char *a, const char *b)
{
	if (a == b)
		return true; /// Both nullptr or same address
	if (!a || !b)
		return false; /// One is nullptr, the other isn't
	return strcmp(a, b) == 0; /// Both valid strings
}

/**
 * @brief Safe equality check for primitives and strings.
 *
 * Handles strict string comparison (strcmp) if types are char*,
 * Automatically handles nullptr checks for strings.
 *
 * @note Use uintptr_t to cast type to pass _Generic type check.
 */
#define eq(a, b)                                                          \
	_Generic((a),                                                     \
		char *: _safe_str_eq((const char *)(uintptr_t)(a),        \
				     (const char *)(uintptr_t)(b)),       \
		const char *: _safe_str_eq((const char *)(uintptr_t)(a),  \
					   (const char *)(uintptr_t)(b)), \
		default: ((a) == (b)))
