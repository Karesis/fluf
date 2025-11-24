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
