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

/// for byte
#include <core/type.h>
/// for offsetof
#include <stddef.h>
/// for bool
#include <stdbool.h>

/// clang builtin types
/// these are return values for __builtin_classify_type
#define _TYPE_VOID 0
#define _TYPE_INTEGER 1 /// char, short, int, long, long long, bool...
#define _TYPE_CHAR 2 /// usually treated as integer in clang (?)
#define _TYPE_ENUM 3
#define _TYPE_BOOL 4
#define _TYPE_POINTER 5
#define _TYPE_REFERENCE 6 /// C++ reference
#define _TYPE_OFFSET 7
#define _TYPE_REAL 8 /// float, double, long double
#define _TYPE_COMPLEX 9
#define _TYPE_FUNCTION 10
#define _TYPE_METHOD 11
#define _TYPE_RECORD 12 /// struct
#define _TYPE_UNION 13
#define _TYPE_ARRAY 14 /// DO NOT USE THIS!! (array will decays to pointer)

/**
 * @brief Check if x is a POINTER variable (excludes arrays!).
 *
 * @note Returns FALSE for string literals (e.g., "abc") because they are ARRAYS (char[]), not pointers.
 * Use is_array() for string literals.
 *
 * Logic:
 * 1. First, check if classify_type thinks it's a pointer (Class 5).
 * (This covers pointers AND arrays, but excludes ints, structs, etc.)
 * 2. If Class 5, use types_compatible_p to distinguish Pointer vs Array.
 * - Pointers: typeof(x) == typeof(x + 0)
 * - Arrays:   typeof(x) != typeof(x + 0) (Array vs Decayed Pointer)
 *
 * Use __builtin_choose_expr to prevent syntax errors (like struct + 0)
 * from being evaluated when x is not a pointer-like type.
 */
#define is_pointer(x)                                        \
	(__builtin_choose_expr(                              \
		__builtin_classify_type(x) == _TYPE_POINTER, \
		__builtin_types_compatible_p(typeof(x), typeof((x) + 0)), 0))

/**
 * @brief Check if x is a integer type.
 * Includes: char, short, int, long, long long, bool, enum
 */
#define is_integer(x)                                   \
	(__builtin_classify_type(x) == _TYPE_INTEGER || \
	 __builtin_classify_type(x) == _TYPE_BOOL) ||   \
		__builtin_classify_type(x) == _TYPE_ENUM

/**
 * @brief Check if x is a floating point type.
 * Includes: float, double, long double.
 */
#define is_floating(x) (__builtin_classify_type(x) == _TYPE_REAL)

/**
 * @brief Check if x is a struct (record) type.
 */
#define is_struct(x) (__builtin_classify_type(x) == _TYPE_RECORD)

/**
 * @brief Check if x is a union type.
 */
#define is_union(x) (__builtin_classify_type(x) == _TYPE_UNION)

/**
 * @brief Check if x is a real ARRAY (not a decayed pointer).
 *
 * Logic:
 * Similar guard as is_pointer, but we look for INCOMPATIBILITY between
 * the type and its decayed form.
 */
#define is_array(x)                                          \
	(__builtin_choose_expr(                              \
		__builtin_classify_type(x) == _TYPE_POINTER, \
		!__builtin_types_compatible_p(typeof(x), typeof((x) + 0)), 0))

/**
 * @brief Check if an expression's type is compatible with a given type.
 */
#define is_type(expr, T) __builtin_types_compatible_p(typeof(expr), T)

/**
 * @brief Check if the two expr has the same type.
 */
#define typecmp(exprA, exprB) \
	__builtin_types_compatible_p(typeof(exprA), typeof(exprB))

/**
 * @brief Get the pointer to the container struct from a pointer to one of its members.
 *
 * @param ptr    A pointer to the 'member' field within the container struct.
 * @param type   The type of the container struct (e.g., struct MyStruct).
 * @param member The name (identifier) of the member field within the container struct.
 *
 * @example
 * /// for assert
 * #include <assert.h>
 * /// for offsetof
 * #include <stddef.h>
 *
 * struct list_node 
 * {
 *		struct list_node *prev;
 *		struct list_node *next;
 * };
 *
 * struct my_data
 * {
 *		int id;
 *		struct list_node node;
 *		char* name;
 * };
 *
 * void container_of_demo()
 * {
 *		struct my_data item_a;
 *		item_a.id = 100;
 *		struct list_node *ptr_to_member = &item_a.node;
 *		struct my_data *ptr_to_container = container_of(ptr_to_member, struct my_data, node);
 *
 *		assert(ptr_to_container == &item_a);
 *		assert(ptr_to_container->id == 100);
 * }
 *
 * @note In C, when we do $ptr - N$, it will be translated to $address(ptr) - (N \times sizeof(type))$.
 * So here the `type` above need to be byte (a.k.a, char) to ensure we use `sizeof(char)` (1) to be the  base unit.
 */
#define container_of(ptr, type, member)                            \
	({                                                         \
		const typeof(((type *)0)->member) *__mptr = (ptr); \
		(type *)((byte *)__mptr - offsetof(type, member)); \
	})

/**
 * @brief Compute the element count of an array.
 *
 * @param arr The array
 */
#define array_size(arr)                                                        \
	({                                                                     \
		static_assert(                                                 \
			is_array(arr),                                         \
			"array_size must be used on an array, not a pointer"); \
		sizeof(arr) / sizeof((arr)[0]);                                \
	})

/**
 * @brief Mark a variable as unused to suppress compiler warnings
 *
 * @param x The unused variable or parameter.
 */
#define unused(x) ((void)(x))

/**
 * @brief Mark a declaration as unused.
 * @see `unused(x) for the statement-based version
 */
#define uattr [[maybe_unused]]

/**
 * @brief Expr do nothing
 */
#define DO_NOTHING ((void)0)

/**
 * @brief Hint to the compiler that the condition is likely true.
 */
#define likely(x) __builtin_expect(!!(x), true)

/**
 * @brief Hint to the compiler that the condition is unlikely true.
 */
#define unlikely(x) __builtin_expect(!!(x), false)

#define alinline __attribute__((always_inline)) inline

#define noinline __attribute__((noinline))

/**
 * @brief RALL
 */
#define defer(fn) __attribute__((cleanup(fn)))
