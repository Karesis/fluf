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

#include <std/fs/srcmanager.h>
#include <core/msg.h>
#include <string.h>

/*
 * ==========================================================================
 * Internal Helpers
 * ==========================================================================
 */

static srcfile_t *_alloc_file(allocer_t alc, str_t filename, str_t content,
                              usize base_offset)
{
    /// 1. alloc Struct
    srcfile_t *f = (srcfile_t *)allocer_alloc(alc, layout_of(srcfile_t));
    if (!f)
        return nullptr;

    /// 2. alloc & Copy Filename (Owned)
    f->filename = (char *)allocer_alloc(alc, layout(filename.len + 1, 1));
    memcpy(f->filename, filename.ptr, filename.len);
    f->filename[filename.len] = '\0';

    /// 3. alloc & Copy Content (Owned)
    f->content = (char *)allocer_alloc(alc, layout(content.len + 1, 1));
    memcpy(f->content, content.ptr, content.len);
    f->content[content.len] = '\0';

    f->len = content.len;
    f->base_offset = base_offset;

    /// 4. build Line Map
    /// initialize vector
    if (!vec_init(f->line_starts, alc, 0)) {
        return nullptr; /// leaking partial f here, but panic in prod usually
    }

    /// line 1 always starts at relative offset 0
    vec_push(f->line_starts, 0);

    /// scan for \n
    for (u32 i = 0; i < (u32)content.len; ++i) {
        if (f->content[i] == '\n') {
            /// next line starts at i + 1
            vec_push(f->line_starts, i + 1);
        }
    }

    return f;
}

static void _free_file(allocer_t alc, srcfile_t *f)
{
    if (!f)
        return;
    if (f->filename)
        allocer_free(alc, f->filename, layout(strlen(f->filename) + 1, 1));
    if (f->content)
        allocer_free(alc, f->content, layout(f->len + 1, 1));
    
    vec_deinit(f->line_starts);
    allocer_free(alc, f, layout_of(srcfile_t));
}

/**
 * @brief Binary search to find the file containing the offset.
 * We want the *last* file where file.base_offset <= global_offset.
 */
static isize _find_file_idx(const srcmanager_t *mgr, usize offset)
{
    if (vec_len(mgr->files) == 0)
        return -1;

    isize left = 0;
    isize right = (isize)vec_len(mgr->files) - 1;
    isize result = -1;

    while (left <= right) {
        isize mid = left + (right - left) / 2;
        srcfile_t *f = vec_at(mgr->files, mid);

        if (f->base_offset <= offset) {
            result = mid; /// candidate found, try to find a later one
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return result;
}

/**
 * @brief Binary search to find line index.
 * We want the *last* line_start <= local_offset.
 */
static isize _find_line_idx(const srcfile_t *f, u32 local_offset)
{
    if (vec_len(f->line_starts) == 0)
        return -1; /// should not happen logic-wise

    isize left = 0;
    isize right = (isize)vec_len(f->line_starts) - 1;
    isize result = 0;

    while (left <= right) {
        isize mid = left + (right - left) / 2;
        u32 start = vec_at(f->line_starts, mid);

        if (start <= local_offset) {
            result = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return result;
}

/*
 * ==========================================================================
 * Lifecycle
 * ==========================================================================
 */

bool srcmanager_init(srcmanager_t *mgr, allocer_t alc)
{
    mgr->alc = alc;
    mgr->total_size = 0;
    return vec_init(mgr->files, alc, 4);
}

void srcmanager_deinit(srcmanager_t *mgr)
{
    vec_foreach(f_ptr, mgr->files) {
        _free_file(mgr->alc, *f_ptr);
    }
    vec_deinit(mgr->files);
}

srcmanager_t *srcmanager_new(allocer_t alc)
{
    srcmanager_t *mgr = (srcmanager_t *)allocer_alloc(alc, layout_of(srcmanager_t));
    if (mgr) {
        if (!srcmanager_init(mgr, alc)) {
            allocer_free(alc, mgr, layout_of(srcmanager_t));
            return nullptr;
        }
    }
    return mgr;
}

void srcmanager_drop(srcmanager_t *mgr)
{
    if (mgr) {
        allocer_t alc = mgr->alc;
        srcmanager_deinit(mgr);
        allocer_free(alc, mgr, layout_of(srcmanager_t));
    }
}

/*
 * ==========================================================================
 * File Management
 * ==========================================================================
 */

usize srcmanager_add(srcmanager_t *mgr, str_t filename, str_t content)
{
    /// base offset is the current total end
    usize base = mgr->total_size;

    srcfile_t *f = _alloc_file(mgr->alc, filename, content, base);
    if (!f)
        return (usize)-1;

    if (!vec_push(mgr->files, f)) {
        _free_file(mgr->alc, f);
        return (usize)-1;
    }

    mgr->total_size += f->len;
    return vec_len(mgr->files) - 1; /// return ID
}

const srcfile_t *srcmanager_get_file(const srcmanager_t *mgr, usize id)
{
    if (id >= vec_len(mgr->files))
        return nullptr;
    return vec_at(mgr->files, id);
}

/*
 * ==========================================================================
 * Resolution
 * ==========================================================================
 */

bool srcmanager_lookup(const srcmanager_t *mgr, usize offset, srcloc_t *out)
{
    /// 1. find File
    isize file_idx = _find_file_idx(mgr, offset);
    if (file_idx < 0) return false;

    srcfile_t *f = vec_at(mgr->files, file_idx);

    /// check if offset is actually within this file's range
    /// (It might be in the gap or beyond the last file if we supported gaps)
    /// here total_size assumes contiguous, but let's be safe.
    if (offset >= f->base_offset + f->len) {
        return false; /// out of bounds of this file
    }

    /// 2. calc Local Offset
    u32 local_offset = (u32)(offset - f->base_offset);

    /// 3. find Line
    isize line_idx = _find_line_idx(f, local_offset);
    
    /// 4. calc Column
    u32 line_start = vec_at(f->line_starts, line_idx);
    u32 col_idx = local_offset - line_start;

    /// fill result
    out->filename = f->filename;
    out->line = (usize)(line_idx + 1); /// 1-based
    out->col = (usize)(col_idx + 1);   /// 1-based

    return true;
}

str_t srcmanager_get_line_content(const srcmanager_t *mgr, usize offset)
{
    srcloc_t loc;
    if (!srcmanager_lookup(mgr, offset, &loc)) {
        return str("");
    }

    /// we need the file again (lookup logic duplication or reuse?)
    /// for speed, we can reuse the logic since we know the line index now is loc.line - 1
    isize file_idx = _find_file_idx(mgr, offset);
    srcfile_t *f = vec_at(mgr->files, file_idx);
    
    usize line_idx = loc.line - 1;
    u32 start_idx = vec_at(f->line_starts, line_idx);
    
    u32 end_idx;
    if (line_idx + 1 < vec_len(f->line_starts)) {
        end_idx = vec_at(f->line_starts, line_idx + 1);
        /// the next line start includes the previous \n, so decrement end by 1?
        /// actually line_start[i+1] points to char AFTER \n.
        /// so the current line goes from start_idx up to line_start[i+1] - 1.
        end_idx -= 1; 
    } else {
        /// last line, goes to end of file
        end_idx = (u32)f->len;
    }

    /// edge case: Windows \r\n. if char at end_idx-1 is \r, trim it.
    if (end_idx > start_idx && f->content[end_idx - 1] == '\r') {
        end_idx--;
    }

    return str_from_parts(f->content + start_idx, end_idx - start_idx);
}
