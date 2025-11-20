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

/// for layout_t
#include <core/mem/layout.h>
/// for min
#include <core/math.h>
/// for anyptr
#include <core/type.h>

/// for massert
#include <core/msg.h>
/// for memset, memcpy
#include <string.h>

/**
 * @brief The V-Table of the allocator.
 */
typedef struct AllocerVtable {
	/// @brief Alloc a block of memory. (Mandatory)
	anyptr (*alloc)(anyptr self, layout_t layout);

	/// @brief Free the memory. (Mandatory)
	void (*free)(anyptr self, anyptr ptr, layout_t layout);

	/// @brief Realloc a block of memory. (Optional)
	/// @note If nullptr, `allocer_realloc` will use generic alloc+memcpy+free.

	anyptr (*realloc)(anyptr self, anyptr ptr, layout_t old_layout,
			  layout_t new_layout);

	/// @brief Alloc a block of memory with zeros. (Optional)
	/// @note If nullptr, `allocer_zalloc` will use alloc+memset.
	anyptr (*zalloc)(anyptr self, layout_t layout);
} allocer_vtable_t;

/**
 * @brief An allocator fat pointer.
 *
 * Combines the allocator state (self) and its behavior (vtable).
 */
typedef struct Allocer {
	anyptr self;
	const allocer_vtable_t *vtable;
} allocer_t;

/*
 * ==========================================================================
 * API Wrappers
 * ==========================================================================
 */

/**
 * @brief Alloc memory using vtable.
 */
[[nodiscard]]
static inline anyptr allocer_alloc(allocer_t allocer, layout_t layout)
{
	massert(allocer.vtable && allocer.vtable->alloc,
		"Allocer vtable or alloc fn is nullptr");
	return allocer.vtable->alloc(allocer.self, layout);
}

/**
 * @brief Free memory using vtable. 
 *
 * @note If the input ptr is nullptr, this will do nothing and just return.
 */
static inline void allocer_free(allocer_t allocer, anyptr ptr, layout_t layout)
{
	/// check before the allocer's free function
	if (ptr == nullptr)
		return;

	massert(allocer.vtable && allocer.vtable->free,
		"Allocer vtable or free fn is nullptr");
	allocer.vtable->free(allocer.self, ptr, layout);
}

/**
 * @brief Zalloc memory using vtable (with fallback).
 */
[[nodiscard]]
static inline anyptr allocer_zalloc(allocer_t allocer, layout_t layout)
{
	massert(allocer.vtable && allocer.vtable->alloc, "Allocer invalid");

	if (allocer.vtable->zalloc) {
		return allocer.vtable->zalloc(allocer.self, layout);
	}

	/// Fallback: alloc + memset
	anyptr ptr = allocer.vtable->alloc(allocer.self, layout);
	if (ptr) {
		memset(ptr, 0, layout.size);
	}
	return ptr;
}

/**
 * @brief Realloc memory using vtable (with fallback).
 */
[[nodiscard]]
static inline anyptr allocer_realloc(allocer_t allocer, anyptr ptr,
				     layout_t old_layout, layout_t new_layout)
{
	massert(allocer.vtable, "Allocer invalid");

	/// if has realloc
	if (allocer.vtable->realloc) {
		return allocer.vtable->realloc(allocer.self, ptr, old_layout,
					       new_layout);
	}

	/// else (fallback to alloc + memcpy + free)
	if (ptr == nullptr) {
		return allocer_alloc(allocer, new_layout);
	}

	if (new_layout.size == 0) {
		allocer_free(allocer, ptr, old_layout);
		return nullptr;
	}

	anyptr new_ptr = allocer_alloc(allocer, new_layout);
	if (new_ptr) {
		usize copy_size = min(new_layout.size, old_layout.size);
		memcpy(new_ptr, ptr, copy_size);
		allocer_free(allocer, ptr, old_layout);
	}
	return new_ptr;
}

/*
 * ============================================================================
 * Type-Safe Macros (Syntax Sugar)
 * ============================================================================
 */

/**
 * @brief Allocate memory for a specific type `T`.
 * @return ptr to T (or nullptr)
 */
#define alloc_type(allocer, T) (T *)allocer_alloc(allocer, layout_of(T))

/**
 * @brief Allocate memory for an array of `T`.
 * @return ptr to T (or nullptr)
 */
#define alloc_array(allocer, T, count) \
	(T *)allocer_alloc(allocer, layout_of_array(T, count))

/**
 * @brief Allocate and zero-initialize memory for type `T`.
 */
#define zalloc_type(allocer, T) (T *)allocer_zalloc(allocer, layout_of(T))

/**
 * @brief Allocate and zero-initialize an array of `T`.
 */
#define zalloc_array(allocer, T, count) \
	(T *)allocer_zalloc(allocer, layout_of_array(T, count))

/**
 * @brief Free a typed pointer.
 */
#define free_type(allocer, ptr) \
	allocer_free(allocer, ptr, layout_of(typeof(*(ptr))))

/**
 * @brief Free a typed array.
 * @note You must provide the same count used during allocation!
 */
#define free_array(allocer, ptr, count) \
	allocer_free(allocer, ptr, layout_of_array(typeof(*(ptr)), count))
