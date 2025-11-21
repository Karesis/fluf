#pragma once

#include <core/mem/allocer.h>

/*
 * ==========================================================================
 * System Allocator
 * ==========================================================================
 * A wrapper around the platform's heap allocator (malloc/free/posix_memalign).
 *
 * - Thread Safe: Yes (relies on C runtime lock).
 * - Alignment: Fully supported (using platform specific aligned allocs).
 * - State: Stateless (self is NULL).
 */

/**
 * @brief Get the global system allocator.
 * * @note This allocator calls OS/Libc primitives directly.
 * On Windows: _aligned_malloc / _aligned_free
 * On POSIX: posix_memalign / free
 */
allocer_t allocer_system(void);
