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
#include <std/hashmap/strhashmap.h>
#include <stdint.h>
#include <string.h>

#define HASHMAP_DEFAULT_CAPACITY 8
#define HASHMAP_MAX_LOAD_FACTOR 0.75

#define TOMBSTONE ((char *)1)

typedef struct {
  char *key;

  void *value;
} Entry;

struct strhashmap {
  allocer_t *alc;
  Entry *entries;
  size_t capacity;
  size_t count;
};

static uint64_t hash_string(const char *key) {
  uint64_t hash = 5381;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

/**
 * @brief 核心查找函数 (复刻自 libkx 的 find_entry)
 * * @param entries 槽位数组
 * @param capacity 总容量
 * @param key 要查找的键
 * @param out_index [out] 指向最终索引的指针 (用于插入或已找到)
 * @return true (已找到) 或 false (未找到, out_index 是插入点)
 */
static bool find_entry(Entry *entries, size_t capacity, const char *key,
                       size_t *out_index) {
  uint64_t hash = hash_string(key);
  size_t index = (size_t)(hash % (uint64_t)capacity);
  size_t first_tombstone = (size_t)-1;

  for_range(i, 0, capacity) {
    size_t current_index = (index + i) % capacity;
    Entry *entry = &entries[current_index];

    if (entry->key == NULL) {

      *out_index =
          (first_tombstone != (size_t)-1) ? first_tombstone : current_index;
      return false;
    }

    if (entry->key == TOMBSTONE) {

      if (first_tombstone == (size_t)-1) {
        first_tombstone = current_index;
      }
      continue;
    }

    if (strcmp(entry->key, key) == 0) {

      *out_index = current_index;
      return true;
    }
  }

  asrt_msg(first_tombstone != (size_t)-1,
           "Hashmap full and no tombstone found (should be resized)");
  *out_index = first_tombstone;
  return false;
}

/**
 * @brief 内部扩容函数
 */
static bool resize(struct strhashmap *map) {
  size_t old_capacity = map->capacity;
  Entry *old_entries = map->entries;

  size_t new_capacity =
      (old_capacity == 0) ? HASHMAP_DEFAULT_CAPACITY : old_capacity * 2;

  layout_t layout = layout_of_array(Entry, new_capacity);
  Entry *new_entries = allocer_zalloc(map->alc, layout);
  if (!new_entries)
    return false;

  map->entries = new_entries;
  map->capacity = new_capacity;
  map->count = 0;

  for_range(i, 0, old_capacity) {
    Entry *entry = &old_entries[i];
    if (entry->key != NULL && entry->key != TOMBSTONE) {

      size_t index;
      bool found = find_entry(new_entries, new_capacity, entry->key, &index);
      asrt_msg(!found, "Resize re-hash failed (key duplicated)");

      new_entries[index].key = entry->key;
      new_entries[index].value = entry->value;
      map->count++;
    }
  }

  allocer_free(map->alc, old_entries, layout_of_array(Entry, old_capacity));
  return true;
}

struct strhashmap *strhashmap_new(allocer_t *alc, size_t initial_capacity) {
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  struct strhashmap *map = allocer_alloc(alc, layout_of(struct strhashmap));
  if (!map)
    return NULL;

  map->alc = alc;
  map->count = 0;
  map->capacity =
      (initial_capacity > 0) ? initial_capacity : HASHMAP_DEFAULT_CAPACITY;

  layout_t layout = layout_of_array(Entry, map->capacity);
  map->entries = allocer_zalloc(map->alc, layout);

  if (!map->entries) {
    allocer_free(map->alc, map, layout_of(struct strhashmap));
    return NULL;
  }

  return map;
}

void strhashmap_free(struct strhashmap *map) {
  if (!map)
    return;

  for_range(i, 0, map->capacity) {
    Entry *entry = &map->entries[i];
    if (entry->key != NULL && entry->key != TOMBSTONE) {

      size_t len = strlen(entry->key);
      allocer_free(map->alc, entry->key, layout_of_array(char, len + 1));
    }
  }

  allocer_free(map->alc, map->entries, layout_of_array(Entry, map->capacity));

  allocer_free(map->alc, map, layout_of(struct strhashmap));
}

bool strhashmap_put(struct strhashmap *map, const char *key, void *value) {
  asrt_msg(key != NULL && key != TOMBSTONE, "Key cannot be NULL or TOMBSTONE");

  if (map->count + 1 > map->capacity * HASHMAP_MAX_LOAD_FACTOR) {
    if (!resize(map))
      return false;
  }

  size_t index;
  bool found = find_entry(map->entries, map->capacity, key, &index);

  if (found) {

    map->entries[index].value = value;

  } else {

    size_t len = strlen(key);
    layout_t key_layout = layout_of_array(char, len + 1);
    char *key_copy = allocer_alloc(map->alc, key_layout);
    if (!key_copy)
      return false;
    memcpy(key_copy, key, len + 1);

    map->count++;

    map->entries[index].key = key_copy;
    map->entries[index].value = value;
  }

  return true;
}

void *strhashmap_get(struct strhashmap *map, const char *key) {
  void *out_value = NULL;
  strhashmap_get_ptr(map, key, &out_value);
  return out_value;
}

bool strhashmap_get_ptr(struct strhashmap *map, const char *key,
                        void **out_value) {
  asrt_msg(key != NULL && key != TOMBSTONE, "Key cannot be NULL");

  if (map->count == 0)
    return false;

  size_t index;
  bool found = find_entry(map->entries, map->capacity, key, &index);

  if (found) {
    *out_value = map->entries[index].value;
    return true;
  } else {
    return false;
  }
}

bool strhashmap_delete(struct strhashmap *map, const char *key) {
  asrt_msg(key != NULL && key != TOMBSTONE, "Key cannot be NULL");

  if (map->count == 0)
    return false;

  size_t index;
  bool found = find_entry(map->entries, map->capacity, key, &index);

  if (found) {

    Entry *entry = &map->entries[index];

    size_t len = strlen(entry->key);
    allocer_free(map->alc, entry->key, layout_of_array(char, len + 1));

    entry->key = TOMBSTONE;
    entry->value = NULL;

    map->count--;
    return true;
  } else {
    return false;
  }
}

void strhashmap_clear(struct strhashmap *map) {

  for_range(i, 0, map->capacity) {
    Entry *entry = &map->entries[i];
    if (entry->key != NULL && entry->key != TOMBSTONE) {

      size_t len = strlen(entry->key);
      allocer_free(map->alc, entry->key, layout_of_array(char, len + 1));
    }
    entry->key = NULL;
    entry->value = NULL;
  }
  map->count = 0;
}

size_t strhashmap_count(const struct strhashmap *map) { return map->count; }