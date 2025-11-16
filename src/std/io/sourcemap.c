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

#include <core/mem/layout.h>
#include <core/msg/asrt.h>
#include <std/io/sourcemap.h>
#include <std/vec.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief 存储单个已加载文件及其行查找表
 */
typedef struct {
  const char *filename;
  str_slice_t content;
  vec_t line_starts;
  size_t base_offset;
  size_t length;
} SourceFile;

bool sourcemap_init(sourcemap_t *map, allocer_t *alc) {
  asrt_msg(map != NULL, "Sourcemap pointer cannot be NULL");
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  map->alc = alc;
  map->total_size = 0;

  return vec_init(&map->files, alc, 0);
}

void sourcemap_destroy(sourcemap_t *map) {
  if (!map)
    return;

  for (size_t i = 0; i < vec_count(&map->files); i++) {
    SourceFile *file = vec_get(&map->files, i);
    if (file) {

      allocer_free(map->alc, (void *)file->filename,
                   layout_of_array(char, strlen(file->filename) + 1));

      vec_destroy(&file->line_starts);

      allocer_free(map->alc, file, layout_of(SourceFile));
    }
  }

  vec_destroy(&map->files);
  memset(map, 0, sizeof(sourcemap_t));
}

size_t sourcemap_add_file(sourcemap_t *map, const char *filename,
                          str_slice_t slice) {

  SourceFile *file = allocer_alloc(map->alc, layout_of(SourceFile));
  if (!file)
    return (size_t)-1;

  size_t filename_len = strlen(filename);
  layout_t name_layout = layout_of_array(char, filename_len + 1);
  char *filename_copy = allocer_alloc(map->alc, name_layout);
  if (!filename_copy) {
    allocer_free(map->alc, file, layout_of(SourceFile));
    return (size_t)-1;
  }
  memcpy(filename_copy, filename, filename_len + 1);

  if (!vec_init(&file->line_starts, map->alc, 0)) {
    allocer_free(map->alc, filename_copy, name_layout);
    allocer_free(map->alc, file, layout_of(SourceFile));
    return (size_t)-1;
  }

  file->filename = filename_copy;
  file->content = slice;
  file->length = slice.len;
  file->base_offset = map->total_size;

  vec_push(&file->line_starts, (void *)(uintptr_t)0);

  for (size_t i = 0; i < slice.len; i++) {
    if (slice.ptr[i] == '\n') {

      vec_push(&file->line_starts, (void *)(uintptr_t)(i + 1));
    }
  }

  vec_push(&map->files, file);
  map->total_size += file->length;

  return vec_count(&map->files) - 1;
}

/**
 * @brief (内部) 在 `line_starts` (一个 `vec_t`) 中二分查找
 * * 找到 `offset` 所在的行索引
 */
static size_t find_line_index(const vec_t *lines, size_t offset) {
  size_t low = 0;
  size_t high = vec_count(lines);
  size_t line_index = 0;

  while (low < high) {
    size_t mid = low + (high - low) / 2;
    size_t mid_offset = (uintptr_t)vec_get(lines, mid);

    if (mid_offset > offset) {

      high = mid;
    } else {

      line_index = mid;
      low = mid + 1;
    }
  }
  return line_index;
}
/**
 * @brief 获取一个文件在全局偏移量中的起始点。
 */
bool sourcemap_get_file_base_offset(const sourcemap_t *map, size_t file_id,
                                    size_t *out_offset) {
  // 1. (安全检查) file_id 是否在 vec_t 的范围内
  if (file_id >= vec_count(&map->files)) {
    return false;
  }

  // 2. 获取私有的 SourceFile*
  SourceFile *file = (SourceFile *)vec_get(&map->files, file_id);
  if (file == NULL) {
    return false; // (理论上不应该发生)
  }

  // 3. 返回 base_offset
  *out_offset = file->base_offset;
  return true;
}

bool sourcemap_lookup(const sourcemap_t *map, size_t offset,
                      source_loc_t *out_loc) {
  if (offset >= map->total_size) {
    return false;
  }

  SourceFile *file = NULL;
  for (size_t i = 0; i < vec_count(&map->files); i++) {
    SourceFile *f = vec_get(&map->files, i);
    if (offset >= f->base_offset && offset < f->base_offset + f->length) {
      file = f;
      break;
    }
  }

  if (file == NULL) {

    return false;
  }

  size_t local_offset = offset - file->base_offset;

  size_t line_index = find_line_index(&file->line_starts, local_offset);
  size_t line_start_offset = (uintptr_t)vec_get(&file->line_starts, line_index);

  out_loc->filename = file->filename;
  out_loc->line = line_index + 1;
  out_loc->column = (local_offset - line_start_offset) + 1;

  return true;
}