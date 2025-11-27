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
#include <core/mem/allocer.h>
#include <std/strings/str.h>
#include <std/allocers/bump.h>
#include <std/vec.h>
#include <std/map.h>

/*
 * ==========================================================================
 * 1. Types
 * ==========================================================================
 */

/**
 * @brief A unique identifier for an interned string.
 * Using a specific type prevents accidental arithmetic or confusion with other ints.
 */
typedef struct Symbol {
	u32 id;
} symbol_t;

/**
 * @brief Helper to compare symbols.
 */
static inline bool sym_eq(symbol_t a, symbol_t b)
{
	return a.id == b.id;
}

/**
 * @brief Helper to check if symbol is valid (assuming 0 is valid, but maybe u32_max is invalid).
 * Let's assume all symbols generated are valid.
 */

/// define the specific Map type: Key=str_t, Value=symbol_t
/// we need this definition to embed it in the Interner struct.
defMap(str_t, symbol_t, StrMap);

/// define the specific Vec type: Elem=str_t (or const char*)
/// storing str_t in vec allows O(1) len retrieval during resolve.
defVec(str_t, StrVec);

/**
 * @brief String Interner.
 *
 * Data structure:
 * 1. `pool`: A bump allocator to store the actual string bytes (stable addresses).
 * 2. `map`:  str_t -> symbol_t (For deduplication/interning).
 * 3. `vec`:  symbol_t -> str_t (For resolution/lookup).
 */
typedef struct Interner {
	bump_t pool; /// owns the string memory
	StrMap map; /// fast lookup (deduplication)
	StrVec vec; /// fast reverse lookup (id -> str)
} interner_t;

/*
 * ==========================================================================
 * 2. Public API
 * ==========================================================================
 */

/**
 * @brief Initialize the interner.
 * @param alc The backing allocator (used by internal Bump, Map, and Vec).
 */
[[nodiscard]] bool intern_init(interner_t *it, allocer_t alc);

/**
 * @brief Destroy the interner and free all memory.
 */
void intern_deinit(interner_t *it);

/**
 * @brief Declare an interner with RAII lifecycle.
 */
#define intern_let(var_name, allocator)                   \
	defer(intern_deinit) interner_t var_name = { 0 }; \
	massert(intern_init(&(var_name), allocator), "Interner init failed")

/**
 * @brief Create a new interner on the heap.
 */
[[nodiscard]] interner_t *intern_new(allocer_t alc);

/**
 * @brief Drop a heap-allocated interner.
 */
void intern_drop(interner_t *it);

/**
 * @brief Intern a string slice.
 *
 * Logic:
 * 1. Check if `s` is already in the map. If yes, return existing symbol.
 * 2. If no:
 * a. Allocate memory for `s` in `it->pool` (null-terminated).
 * b. Create a new symbol ID (current vec len).
 * c. Insert into `map`.
 * d. Push into `vec`.
 * e. Return new symbol.
 *
 * @return The symbol. (Panics on OOM currently, or we can discuss result).
 * Let's stick to "return symbol, panic on OOM" for simplicity in compilers,
 * or check validity. For now, assume success or panic inside alloc.
 */
symbol_t intern(interner_t *it, str_t s);

/**
 * @brief Intern a C-style string.
 */
static inline symbol_t intern_cstr(interner_t *it, const char *s)
{
	return intern(it, str_from_cstr(s));
}

/**
 * @brief Resolve a symbol back to a string slice.
 * @note O(1) operation.
 */
str_t intern_resolve(const interner_t *it, symbol_t sym);

/**
 * @brief Resolve a symbol back to a C-string.
 * @note O(1) operation. The string is guaranteed to be null-terminated
 * because we allocated it that way in the pool.
 */
const char *intern_resolve_cstr(const interner_t *it, symbol_t sym);

/**
 * @brief Get the number of unique interned strings.
 */
usize intern_count(const interner_t *it);
