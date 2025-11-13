#include <std/string/strintern.h>
#include <core/mem/allocer.h>
#include <core/mem/layout.h>
#include <stdint.h>
#include <core/msg/asrt.h>
#include <core/span.h>
#include <string.h> // for strlen, memcpy

// --- 内部常量 ---
#define INTERN_DEFAULT_CAPACITY 16
#define INTERN_MAX_LOAD_FACTOR 0.75

// --- 内部哨兵 ---
// 和 strhashmap.c 一样
// key == NULL     -> EMPTY
// key == TOMBSTONE -> DELETED
static const char *TOMBSTONE = (const char *)1;

// --- 内部哈希函数 (djb2 for slices) ---
static uint64_t
hash_slice(str_slice_t slice)
{
  uint64_t hash = 5381;
  for_range(i, 0, slice.len)
  {
    hash = ((hash << 5) + hash) + slice.ptr[i]; // hash * 33 + c
  }
  return hash;
}

// --- 内部辅助函数 ---

/**
 * @brief 核心查找函数
 *
 * @param entries 槽位数组
 * @param capacity 总容量
 * @param key_slice 要查找的 *切片*
 * @param out_index [out] 指向最终索引的指针 (用于插入或已找到)
 * @return true (已找到) 或 false (未找到, out_index 是插入点)
 */
static bool
find_entry(const char **entries, size_t capacity, str_slice_t key_slice, size_t *out_index)
{
  uint64_t hash = hash_slice(key_slice);
  size_t index = (size_t)(hash % (uint64_t)capacity);
  size_t first_tombstone = (size_t)-1; // 哨兵

  for_range(i, 0, capacity)
  {
    size_t current_index = (index + i) % capacity;
    const char *entry_key = entries[current_index];

    if (entry_key == NULL)
    {
      // 找到空槽 (EMPTY)
      *out_index = (first_tombstone != (size_t)-1) ? first_tombstone : current_index;
      return false;
    }

    if (entry_key == TOMBSTONE)
    {
      // 找到墓碑 (DELETED)
      if (first_tombstone == (size_t)-1)
      {
        first_tombstone = current_index;
      }
      continue; // 继续搜索
    }

    // 找到占用槽 (OCCUPIED)
    // 关键比较：我们用 `slice_equals_cstr` 来比较 "脏"切片 和 "干净"C-string
    if (slice_equals_cstr(key_slice, entry_key))
    {
      *out_index = current_index;
      return true;
    }
  }

  asrt_msg(first_tombstone != (size_t)-1, "Interner full and no tombstone found (should be resized)");
  *out_index = first_tombstone;
  return false;
}

/**
 * @brief 内部扩容函数
 */
static bool
resize(strintern_t *interner)
{
  size_t old_capacity = interner->capacity;
  const char **old_entries = interner->entries;

  size_t new_capacity = (old_capacity == 0) ? INTERN_DEFAULT_CAPACITY : old_capacity * 2;

  layout_t layout = layout_of_array(const char *, new_capacity);
  const char **new_entries = allocer_zalloc(interner->alc, layout); // zalloc 保证全 NULL
  if (!new_entries)
    return false; // OOM

  // 2. 更新 interner
  interner->entries = new_entries;
  interner->capacity = new_capacity;
  interner->count = 0; // 重新插入时会加回来

  // 3. Re-hash (重新插入所有旧条目)
  for_range(i, 0, old_capacity)
  {
    const char *entry_key = old_entries[i];
    if (entry_key != NULL && entry_key != TOMBSTONE)
    {
      // 我们需要用旧的 c-string 重新查找
      str_slice_t key_slice = slice_from_cstr(entry_key);

      size_t index;
      bool found = find_entry(new_entries, new_capacity, key_slice, &index);
      asrt_msg(!found, "Resize re-hash failed (key duplicated)");

      new_entries[index] = entry_key; // 只移动指针
      interner->count++;
    }
  }

  // 4. 释放旧的 *entries 数组* (不是字符串)
  allocer_free(interner->alc, old_entries, layout_of_array(const char *, old_capacity));
  return true;
}

// --- 公共 API 实现 ---

bool strintern_init(strintern_t *interner, allocer_t *alc, size_t initial_capacity)
{
  asrt_msg(interner != NULL, "Interner pointer cannot be NULL");
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  interner->alc = alc;
  interner->count = 0;
  interner->capacity = (initial_capacity > 0) ? initial_capacity : INTERN_DEFAULT_CAPACITY;

  layout_t layout = layout_of_array(const char *, interner->capacity);
  interner->entries = allocer_zalloc(interner->alc, layout); // zalloc 保证全 NULL

  if (!interner->entries)
  { // OOM
    return false;
  }

  return true;
}

void strintern_destroy(strintern_t *interner)
{
  if (!interner)
    return;

  // 1. 释放 `entries` 数组
  allocer_free(interner->alc, interner->entries, layout_of_array(const char *, interner->capacity));

  // 2. 将结构体清零 (防止悬空指针)
  interner->entries = NULL;
  interner->alc = NULL;
  interner->capacity = 0;
  interner->count = 0;
}

const char *
strintern_intern_slice(strintern_t *interner, str_slice_t slice)
{
  // 1. 检查是否需要扩容
  if (interner->count + 1 > interner->capacity * INTERN_MAX_LOAD_FACTOR)
  {
    if (!resize(interner))
      return NULL; // OOM
  }

  // 2. 查找槽位
  size_t index;
  bool found = find_entry(interner->entries, interner->capacity, slice, &index);

  // 3. 处理
  if (found)
  {
    // 键已存在 -> 直接返回
    return interner->entries[index];
  }
  else
  {
    // 键不存在 -> 插入新值

    // 3a. 创建一个新的、以 '\0' 结尾的副本
    layout_t key_layout = layout_of_array(char, slice.len + 1);
    char *key_copy = allocer_alloc(interner->alc, key_layout);
    if (!key_copy)
      return NULL; // OOM

    memcpy(key_copy, slice.ptr, slice.len); // 复制内容
    key_copy[slice.len] = '\0';             // 添加 '\0'

    // 3b. 存入新指针
    interner->entries[index] = key_copy;
    interner->count++;

    return key_copy;
  }
}

const char *
strintern_intern_cstr(strintern_t *interner, const char *str)
{
  asrt_msg(str != NULL, "Cannot intern NULL c-string");
  return strintern_intern_slice(interner, slice_from_cstr(str));
}

void strintern_clear(strintern_t *interner)
{
  // 只重置哈希表，不释放 `alc` 分配的字符串
  // (假定用户会 `bump_reset(arena)`)
  memset(interner->entries, 0, sizeof(const char *) * interner->capacity);
  interner->count = 0;
}

size_t
strintern_count(const strintern_t *interner)
{
  return interner->count;
}