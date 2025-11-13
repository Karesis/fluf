#include <std/hashmap/strhashmap.h>
#include <core/mem/allocer.h>
#include <core/mem/layout.h>
#include <stdint.h>
#include <core/msg/asrt.h>
#include <core/span.h>
#include <string.h> // for strcmp, strlen, memcpy

// --- 内部常量 ---
#define HASHMAP_DEFAULT_CAPACITY 8
#define HASHMAP_MAX_LOAD_FACTOR 0.75

// --- 内部状态 ---
// 我们不使用墓碑(DELETED)，而是用一个技巧：
// 当删除时，我们将 `key` 指针设置为一个哨兵值 (TOMBSTONE)。
// 这避免了 `libkx` 中 `HM_STATE_DELETED` 的复杂性。
#define TOMBSTONE ((char *)1)

// --- 内部 Entry 结构 ---
typedef struct
{
  char *key; // 指向 *副本* C 字符串的指针
             // NULL = EMPTY (空)
             // TOMBSTONE = DELETED (墓碑)
             // other = OCCUPIED (占用)
  void *value;
} Entry;

// --- 公开 (Opaque) 结构 ---
struct strhashmap
{
  allocer_t *alc;
  Entry *entries;
  size_t capacity;
  size_t count;
};

// --- 内部哈希函数 (djb2) ---
// 简单、自洽、性能足够好
static uint64_t
hash_string(const char *key)
{
  uint64_t hash = 5381;
  int c;
  while ((c = *key++))
  {
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  }
  return hash;
}

// --- 内部辅助函数 ---

/**
 * @brief 核心查找函数 (复刻自 libkx 的 find_entry)
 * * @param entries 槽位数组
 * @param capacity 总容量
 * @param key 要查找的键
 * @param out_index [out] 指向最终索引的指针 (用于插入或已找到)
 * @return true (已找到) 或 false (未找到, out_index 是插入点)
 */
static bool
find_entry(Entry *entries, size_t capacity, const char *key, size_t *out_index)
{
  uint64_t hash = hash_string(key);
  size_t index = (size_t)(hash % (uint64_t)capacity);
  size_t first_tombstone = (size_t)-1; // 哨兵

  for_range(i, 0, capacity)
  {
    size_t current_index = (index + i) % capacity;
    Entry *entry = &entries[current_index];

    if (entry->key == NULL)
    {
      // 找到空槽 (EMPTY)
      // 如果我们之前路过了墓碑，就用那个墓碑
      *out_index = (first_tombstone != (size_t)-1) ? first_tombstone : current_index;
      return false;
    }

    if (entry->key == TOMBSTONE)
    {
      // 找到墓碑 (DELETED)
      if (first_tombstone == (size_t)-1)
      {
        first_tombstone = current_index;
      }
      continue; // 继续搜索
    }

    // 找到占用槽 (OCCUPIED)
    if (strcmp(entry->key, key) == 0)
    {
      // 键匹配！
      *out_index = current_index;
      return true;
    }
  }

  // 表已满，但我们应该返回第一个墓碑（如果有）
  asrt_msg(first_tombstone != (size_t)-1, "Hashmap full and no tombstone found (should be resized)");
  *out_index = first_tombstone;
  return false;
}

/**
 * @brief 内部扩容函数
 */
static bool
resize(struct strhashmap *map)
{
  size_t old_capacity = map->capacity;
  Entry *old_entries = map->entries;

  size_t new_capacity = (old_capacity == 0) ? HASHMAP_DEFAULT_CAPACITY : old_capacity * 2;

  // 1. 分配新表 (清零)
  layout_t layout = layout_of_array(Entry, new_capacity);
  Entry *new_entries = allocer_zalloc(map->alc, layout);
  if (!new_entries)
    return false; // OOM

  // 2. 更新 map
  map->entries = new_entries;
  map->capacity = new_capacity;
  map->count = 0; // put 会把它加回来

  // 3. Re-hash (重新插入所有旧条目)
  for_range(i, 0, old_capacity)
  {
    Entry *entry = &old_entries[i];
    if (entry->key != NULL && entry->key != TOMBSTONE)
    {
      // 这次我们不*复制* key，我们只*移动*指针
      // (因为 key 已经是我们分配的了)
      size_t index;
      bool found = find_entry(new_entries, new_capacity, entry->key, &index);
      asrt_msg(!found, "Resize re-hash failed (key duplicated)");

      new_entries[index].key = entry->key;
      new_entries[index].value = entry->value;
      map->count++;
    }
  }

  // 4. 释放旧表
  allocer_free(map->alc, old_entries, layout_of_array(Entry, old_capacity));
  return true;
}

// --- 公共 API 实现 ---

struct strhashmap *
strhashmap_new(allocer_t *alc, size_t initial_capacity)
{
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  struct strhashmap *map = allocer_alloc(alc, layout_of(struct strhashmap));
  if (!map)
    return NULL; // OOM

  map->alc = alc;
  map->count = 0;
  map->capacity = (initial_capacity > 0) ? initial_capacity : HASHMAP_DEFAULT_CAPACITY;

  layout_t layout = layout_of_array(Entry, map->capacity);
  map->entries = allocer_zalloc(map->alc, layout); // zalloc 保证所有 key 都是 NULL

  if (!map->entries)
  { // OOM
    allocer_free(map->alc, map, layout_of(struct strhashmap));
    return NULL;
  }

  return map;
}

void strhashmap_free(struct strhashmap *map)
{
  if (!map)
    return;

  // 释放所有复制的键字符串
  for_range(i, 0, map->capacity)
  {
    Entry *entry = &map->entries[i];
    if (entry->key != NULL && entry->key != TOMBSTONE)
    {
      // 释放 key 字符串本身
      size_t len = strlen(entry->key);
      allocer_free(map->alc, entry->key, layout_of_array(char, len + 1));
    }
  }

  // 释放 Entry 数组
  allocer_free(map->alc, map->entries, layout_of_array(Entry, map->capacity));

  // 释放 map 结构体
  allocer_free(map->alc, map, layout_of(struct strhashmap));
}

bool strhashmap_put(struct strhashmap *map, const char *key, void *value)
{
  asrt_msg(key != NULL && key != TOMBSTONE, "Key cannot be NULL or TOMBSTONE");

  // 1. 检查是否需要扩容
  if (map->count + 1 > map->capacity * HASHMAP_MAX_LOAD_FACTOR)
  {
    if (!resize(map))
      return false; // OOM
  }

  // 2. 查找插入槽位
  size_t index;
  bool found = find_entry(map->entries, map->capacity, key, &index);

  // 3. 处理插入/更新
  if (found)
  {
    // 键已存在 -> 更新值
    map->entries[index].value = value;
    // 注意：我们不释放旧的 key，因为 key 是一样的
  }
  else
  {
    // 键不存在 -> 插入新值

    // 3a. 复制键字符串 (使用分配器)
    size_t len = strlen(key);
    layout_t key_layout = layout_of_array(char, len + 1);
    char *key_copy = allocer_alloc(map->alc, key_layout);
    if (!key_copy)
      return false; // OOM
    memcpy(key_copy, key, len + 1);

    // 3b. 插入新条目
    // 无论是覆盖 NULL (空) 还是 TOMBSTONE (墓碑)，
    // 我们都在增加一个"活动"条目，所以 count 必须增加。
    map->count++;

    map->entries[index].key = key_copy;
    map->entries[index].value = value;
  }

  return true;
}

void *
strhashmap_get(struct strhashmap *map, const char *key)
{
  void *out_value = NULL;
  strhashmap_get_ptr(map, key, &out_value);
  return out_value;
}

bool strhashmap_get_ptr(struct strhashmap *map, const char *key, void **out_value)
{
  asrt_msg(key != NULL && key != TOMBSTONE, "Key cannot be NULL");

  if (map->count == 0)
    return false; // 快速路径

  size_t index;
  bool found = find_entry(map->entries, map->capacity, key, &index);

  if (found)
  {
    *out_value = map->entries[index].value;
    return true;
  }
  else
  {
    return false;
  }
}

bool strhashmap_delete(struct strhashmap *map, const char *key)
{
  asrt_msg(key != NULL && key != TOMBSTONE, "Key cannot be NULL");

  if (map->count == 0)
    return false;

  size_t index;
  bool found = find_entry(map->entries, map->capacity, key, &index);

  if (found)
  {
    // 找到了 -> 放置墓碑
    Entry *entry = &map->entries[index];

    // 1. 释放键字符串的内存
    size_t len = strlen(entry->key);
    allocer_free(map->alc, entry->key, layout_of_array(char, len + 1));

    // 2. 设置墓碑
    entry->key = TOMBSTONE;
    entry->value = NULL; // (可选，但很干净)

    map->count--;
    return true;
  }
  else
  {
    return false; // 未找到
  }
}

void strhashmap_clear(struct strhashmap *map)
{
  // 释放所有键并清空所有槽位
  for_range(i, 0, map->capacity)
  {
    Entry *entry = &map->entries[i];
    if (entry->key != NULL && entry->key != TOMBSTONE)
    {
      // 释放 key 字符串
      size_t len = strlen(entry->key);
      allocer_free(map->alc, entry->key, layout_of_array(char, len + 1));
    }
    entry->key = NULL; // 设置为 EMPTY
    entry->value = NULL;
  }
  map->count = 0;
}

size_t
strhashmap_count(const struct strhashmap *map)
{
  return map->count;
}