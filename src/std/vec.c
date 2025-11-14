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
#include <std/vec.h>
#include <string.h>

#define VEC_DEFAULT_CAPACITY 4

/**
 * @brief 内部扩容函数
 */
static bool vec_resize(vec_t *vec, size_t new_capacity) {
  asrt_msg(new_capacity > vec->capacity,
           "New capacity must be larger than old");

  layout_t old_layout = layout_of_array(void *, vec->capacity);
  layout_t new_layout = layout_of_array(void *, new_capacity);

  void *new_data;
  if (vec->capacity == 0) {

    new_data = allocer_alloc(vec->alc, new_layout);
  } else {

    new_data = allocer_realloc(vec->alc, vec->data, old_layout, new_layout);
  }

  if (!new_data) {
    return false;
  }

  vec->data = new_data;
  vec->capacity = new_capacity;
  return true;
}

bool vec_init(vec_t *vec, allocer_t *alc, size_t initial_capacity) {
  asrt_msg(vec != NULL, "Vector pointer cannot be NULL");
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  size_t capacity =
      (initial_capacity > 0) ? initial_capacity : VEC_DEFAULT_CAPACITY;

  layout_t layout = layout_of_array(void *, capacity);
  void **data = allocer_alloc(alc, layout);

  if (!data) {

    memset(vec, 0, sizeof(vec_t));
    return false;
  }

  vec->alc = alc;
  vec->data = data;
  vec->capacity = capacity;
  vec->count = 0;

  return true;
}

void vec_destroy(vec_t *vec) {
  if (!vec)
    return;

  if (vec->data) {
    layout_t layout = layout_of_array(void *, vec->capacity);
    allocer_free(vec->alc, vec->data, layout);
  }

  memset(vec, 0, sizeof(vec_t));
}

bool vec_push(vec_t *vec, void *ptr) {

  if (vec->count == vec->capacity) {
    size_t new_capacity =
        (vec->capacity == 0) ? VEC_DEFAULT_CAPACITY : vec->capacity * 2;
    if (!vec_resize(vec, new_capacity)) {
      return false;
    }
  }

  vec->data[vec->count] = ptr;
  vec->count++;
  return true;
}

void *vec_pop(vec_t *vec) {
  if (vec->count == 0) {
    return NULL;
  }

  vec->count--;

  return vec->data[vec->count];
}

void *vec_get(const vec_t *vec, size_t index) {
  if (index >= vec->count) {
    return NULL;
  }
  return vec->data[index];
}

void vec_set(vec_t *vec, size_t index, void *ptr) {

  asrt_msg(index < vec->count, "vec_set index out of bounds");
  vec->data[index] = ptr;
}

void vec_clear(vec_t *vec) { vec->count = 0; }

size_t vec_count(const vec_t *vec) { return vec->count; }

void **vec_data(const vec_t *vec) { return vec->data; }