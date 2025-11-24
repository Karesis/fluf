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

typedef enum {
	DIR_ENTRY_FILE,
	DIR_ENTRY_DIR,
	DIR_ENTRY_UNKNOWN /// block device, socket, fifo, etc.
} dir_entry_type_t;

/**
 * @brief Callback for directory walking.
 *
 * @param path The full path to the entry (e.g., "src/main.c").
 * NOTE: This pointer is ephemeral (valid only during callback).
 * If you need to keep it, copy it.
 * @param type The type of the entry.
 * @param userdata User context passed to dir_walk.
 * @return true to continue walking, false to abort immediately.
 */
typedef bool (*dir_walk_cb)(const char *path, dir_entry_type_t type,
			    void *userdata);

/**
 * @brief Recursively walk a directory.
 *
 * @param alc Allocator used for the internal path builder.
 * @param root Path to start walking from.
 * @param cb Callback function invoked for each entry.
 * @param userdata Passed to the callback.
 * @return true if completed successfully, false if root is invalid or I/O error.
 */
bool dir_walk(allocer_t alc, const char *root, dir_walk_cb cb, void *userdata);
