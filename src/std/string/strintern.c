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

#include <core/mem/allocer.h>
#include <core/mem/layout.h>
#include <core/msg/asrt.h>
#include <core/span.h>
#include <std/string/strintern.h>
#include <stdint.h>
#include <string.h>

#define INTERN_DEFAULT_CAPACITY 16
#define INTERN_MAX_LOAD_FACTOR 0.75

static const char *TOMBSTONE = (const char *)1;

static uint64_t hash_slice(strslice_t slice) {
  uint64_t hash = 5381;
  for_range(i, 0, slice.len) { hash = ((hash << 5) + hash) + slice.ptr[i]; }
  return hash;
}

/**
 * @brief 核心查找函数
 *
 * @param entries 槽位数组
 * @param capacity 总容量
 * @param key_slice 要查找的 *切片*
 * @param out_index [out] 指向最终索引的指针 (用于插入或已找到)
 * @return true (已找到) 或 false (未找到, out_index 是插入点)
 */
static bool find_entry(const char **entries, size_t capacity,
                       strslice_t key_slice, size_t *out_index) {
  uint64_t hash = hash_slice(key_slice);
  size_t index = (size_t)(hash % (uint64_t)capacity);
  size_t first_tombstone = (size_t)-1;

  for_range(i, 0, capacity) {
    size_t current_index = (index + i) % capacity;
    const char *entry_key = entries[current_index];

    if (entry_key == NULL) {

      *out_index =
          (first_tombstone != (size_t)-1) ? first_tombstone : current_index;
      return false;
    }

    if (entry_key == TOMBSTONE) {

      if (first_tombstone == (size_t)-1) {
        first_tombstone = current_index;
      }
      continue;
    }

    if (slice_equals_cstr(key_slice, entry_key)) {
      *out_index = current_index;
      return true;
    }
  }

  asrt_msg(first_tombstone != (size_t)-1,
           "Interner full and no tombstone found (should be resized)");
  *out_index = first_tombstone;
  return false;
}

/**
 * @brief 内部扩容函数
 */
static bool resize(strintern_t *interner) {
  size_t old_capacity = interner->capacity;
  const char **old_entries = interner->entries;

  size_t new_capacity =
      (old_capacity == 0) ? INTERN_DEFAULT_CAPACITY : old_capacity * 2;

  layout_t layout = layout_of_array(const char *, new_capacity);
  const char **new_entries = allocer_zalloc(interner->alc, layout);
  if (!new_entries)
    return false;

  interner->entries = new_entries;
  interner->capacity = new_capacity;
  interner->count = 0;

  for_range(i, 0, old_capacity) {
    const char *entry_key = old_entries[i];
    if (entry_key != NULL && entry_key != TOMBSTONE) {

      strslice_t key_slice = slice_from_cstr(entry_key);

      size_t index;
      bool found = find_entry(new_entries, new_capacity, key_slice, &index);
      asrt_msg(!found, "Resize re-hash failed (key duplicated)");

      new_entries[index] = entry_key;
      interner->count++;
    }
  }

  allocer_free(interner->alc, old_entries,
               layout_of_array(const char *, old_capacity));
  return true;
}

bool strintern_init(strintern_t *interner, allocer_t *alc,
                    size_t initial_capacity) {
  asrt_msg(interner != NULL, "Interner pointer cannot be NULL");
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  interner->alc = alc;
  interner->count = 0;
  interner->capacity =
      (initial_capacity > 0) ? initial_capacity : INTERN_DEFAULT_CAPACITY;

  layout_t layout = layout_of_array(const char *, interner->capacity);
  interner->entries = allocer_zalloc(interner->alc, layout);

  if (!interner->entries) {
    return false;
  }

  return true;
}

void strintern_destroy(strintern_t *interner) {
  if (!interner)
    return;

  allocer_free(interner->alc, interner->entries,
               layout_of_array(const char *, interner->capacity));

  interner->entries = NULL;
  interner->alc = NULL;
  interner->capacity = 0;
  interner->count = 0;
}

const char *strintern_intern_slice(strintern_t *interner, strslice_t slice) {

  if (interner->count + 1 > interner->capacity * INTERN_MAX_LOAD_FACTOR) {
    if (!resize(interner))
      return NULL;
  }

  size_t index;
  bool found = find_entry(interner->entries, interner->capacity, slice, &index);

  if (found) {

    return interner->entries[index];
  } else {

    layout_t key_layout = layout_of_array(char, slice.len + 1);
    char *key_copy = allocer_alloc(interner->alc, key_layout);
    if (!key_copy)
      return NULL;

    memcpy(key_copy, slice.ptr, slice.len);
    key_copy[slice.len] = '\0';

    interner->entries[index] = key_copy;
    interner->count++;

    return key_copy;
  }
}

const char *strintern_intern_cstr(strintern_t *interner, const char *str) {
  asrt_msg(str != NULL, "Cannot intern NULL c-string");
  return strintern_intern_slice(interner, slice_from_cstr(str));
}

void strintern_clear(strintern_t *interner) {

  memset(interner->entries, 0, sizeof(const char *) * interner->capacity);
  interner->count = 0;
}

size_t strintern_count(const strintern_t *interner) { return interner->count; }