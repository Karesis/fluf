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
#include <std/vec.h>
#include <std/strings/str.h>

/*
 * ==========================================================================
 * 1. Types
 * ==========================================================================
 */

typedef struct SourceLocation {
	const char *filename; /// pointer to the owned string in srcfile_t
	usize line; /// 1-based
	usize col; /// 1-based
} srcloc_t;

typedef struct SourceFile {
	char *filename; /// owned string (allocated in manager's arena/heap)
	char *content; /// owned content (null terminated)
	usize len; /// content length
	usize base_offset; /// global start offset in the manager space
	vec(u32) line_starts; /// offsets of each line start (relative to file start)
} srcfile_t;

defVec(srcfile_t *, SourceFileVec);

typedef struct SourceManager {
	SourceFileVec files; /// all registered files (sorted by base_offset)
	usize total_size; /// total bytes of all files combined (next base_offset)
	allocer_t alc; /// backing allocator
} srcmanager_t;

/*
 * ==========================================================================
 * 2. Lifecycle
 * ==========================================================================
 */

[[nodiscard]] bool srcmanager_init(srcmanager_t *mgr, allocer_t alc);
void srcmanager_deinit(srcmanager_t *mgr);

[[nodiscard]] srcmanager_t *srcmanager_new(allocer_t alc);
void srcmanager_drop(srcmanager_t *mgr);

/*
 * ==========================================================================
 * 3. File Management
 * ==========================================================================
 */

/**
 * @brief Add a file content to the manager.
 *
 * @param filename Name of the file (copied internally).
 * @param content  Content of the file (copied internally).
 * @return File ID (index in the files vector), or (usize)-1 on failure.
 * * @note This calculates line endings immediately to build `line_starts`.
 */
usize srcmanager_add(srcmanager_t *mgr, str_t filename, str_t content);

/**
 * @brief Get a file by ID.
 */
const srcfile_t *srcmanager_get_file(const srcmanager_t *mgr, usize id);

/*
 * ==========================================================================
 * 4. Location Resolution
 * ==========================================================================
 */

/**
 * @brief Resolve a global offset to a SourceLocation (File, Line, Col).
 * * logic:
 * 1. Binary search `mgr->files` to find which file contains the offset.
 * 2. Calculate `local_offset = offset - file->base_offset`.
 * 3. Binary search `file->line_starts` to find the line number.
 * 4. Calculate column from the line start.
 * * @param out [out] Result struct.
 * @return true if found, false if offset is out of bounds.
 */
bool srcmanager_lookup(const srcmanager_t *mgr, usize offset, srcloc_t *out);

/**
 * @brief Get the line content for a given location.
 * Useful for printing error messages with context code snippet.
 * * e.g., if offset points to "int x = 0;", this returns the whole line slice.
 */
str_t srcmanager_get_line_content(const srcmanager_t *mgr, usize offset);
