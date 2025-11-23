#pragma once

#include <core/type.h>

/*
 * ==========================================================================
 * FNV-1a Hash Implementation (64-bit)
 * ==========================================================================
 */

#define FNV_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV_PRIME 0x100000001b3ULL

/**
 * @brief Compute FNV-1a hash for a byte buffer.
 */
static inline u64 hash_bytes(const void *data, usize len)
{
	u64 hash = FNV_OFFSET_BASIS;
	const u8 *bytes = (const u8 *)data;

	for (usize i = 0; i < len; ++i) {
		hash ^= bytes[i];
		hash *= FNV_PRIME;
	}
	return hash;
}

/**
 * @brief Helper for hashing integers/pointers (by value representation).
 */
#define hash_val(x) hash_bytes(&(x), sizeof(x))
