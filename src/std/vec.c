#include <std/vec.h>
#include <core/mem/allocer.h>
#include <core/mem/layout.h>
#include <core/msg/asrt.h>
#include <string.h> // for memset

// --- 内部常量 ---
#define VEC_DEFAULT_CAPACITY 4

// --- 内部辅助函数 ---

/**
 * @brief 内部扩容函数
 */
static bool
vec_resize(vec_t *vec, size_t new_capacity)
{
  asrt_msg(new_capacity > vec->capacity, "New capacity must be larger than old");

  // 1. 定义新旧布局
  layout_t old_layout = layout_of_array(void *, vec->capacity);
  layout_t new_layout = layout_of_array(void *, new_capacity);

  // 2. 重新分配
  void *new_data;
  if (vec->capacity == 0)
  {
    // 之前是空的，我们必须 `alloc`
    new_data = allocer_alloc(vec->alc, new_layout);
  }
  else
  {
    // 之前有数据，我们 `realloc`
    new_data = allocer_realloc(vec->alc, vec->data, old_layout, new_layout);
  }

  if (!new_data)
  {
    return false; // OOM
  }

  // 3. 更新 vec
  vec->data = new_data;
  vec->capacity = new_capacity;
  return true;
}

// --- 公共 API 实现 ---

bool vec_init(vec_t *vec, allocer_t *alc, size_t initial_capacity)
{
  asrt_msg(vec != NULL, "Vector pointer cannot be NULL");
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  size_t capacity = (initial_capacity > 0) ? initial_capacity : VEC_DEFAULT_CAPACITY;

  // 1. 分配初始内存
  layout_t layout = layout_of_array(void *, capacity);
  void **data = allocer_alloc(alc, layout);

  if (!data)
  {
    // OOM 时，确保结构体处于安全状态
    memset(vec, 0, sizeof(vec_t));
    return false;
  }

  // 2. 初始化结构体
  vec->alc = alc;
  vec->data = data;
  vec->capacity = capacity;
  vec->count = 0;

  return true;
}

void vec_destroy(vec_t *vec)
{
  if (!vec)
    return;

  // 1. 释放 `data` 数组
  if (vec->data)
  {
    layout_t layout = layout_of_array(void *, vec->capacity);
    allocer_free(vec->alc, vec->data, layout);
  }

  // 2. 将结构体清零 (防止悬空指针)
  memset(vec, 0, sizeof(vec_t));
}

bool vec_push(vec_t *vec, void *ptr)
{
  // 1. 检查是否需要扩容
  if (vec->count == vec->capacity)
  {
    size_t new_capacity = (vec->capacity == 0) ? VEC_DEFAULT_CAPACITY : vec->capacity * 2;
    if (!vec_resize(vec, new_capacity))
    {
      return false; // OOM on resize
    }
  }

  // 2. 插入元素
  vec->data[vec->count] = ptr;
  vec->count++;
  return true;
}

void *
vec_pop(vec_t *vec)
{
  if (vec->count == 0)
  {
    return NULL; // Vector 为空
  }

  // 1. 减少 count
  vec->count--;

  // 2. 返回之前的最后一个元素
  return vec->data[vec->count];
}

void *
vec_get(const vec_t *vec, size_t index)
{
  if (index >= vec->count)
  {
    return NULL; // 索引越界
  }
  return vec->data[index];
}

void vec_set(vec_t *vec, size_t index, void *ptr)
{
  // 在 Debug 模式下，如果越界则立即崩溃
  asrt_msg(index < vec->count, "vec_set index out of bounds");
  vec->data[index] = ptr;
}

void vec_clear(vec_t *vec)
{
  vec->count = 0;
}

size_t
vec_count(const vec_t *vec)
{
  return vec->count;
}

void **
vec_data(const vec_t *vec)
{
  return vec->data;
}