#include <std/allocer/bump/bump.h>
#include <core/mem/layout.h>
#include <core/msg/asrt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 为了可移植地处理对齐内存分配：
 * - Windows: _aligned_malloc / _aligned_free
 * - POSIX: posix_memalign / free
 */
#if defined(_WIN32)
#include <malloc.h>
static void *
aligned_malloc_internal(size_t alignment, size_t size)
{
  return _aligned_malloc(size, alignment);
}
static void
aligned_free_internal(void *ptr)
{
  _aligned_free(ptr);
}
#else

static void *
aligned_malloc_internal(size_t alignment, size_t size)
{
  void *ptr = NULL;
  if (posix_memalign(&ptr, alignment, size) != 0)
  {
    return NULL;
  }
  return ptr;
}
static void
aligned_free_internal(void *ptr)
{
  free(ptr);
}
#endif

/*
 * --- 对齐和常量 ---
 */

static bool
is_power_of_two(size_t n)
{
  return (n != 0) && ((n & (n - 1)) == 0);
}

static size_t
round_up_to(size_t n, size_t divisor)
{
  asrt(is_power_of_two(divisor));
  return (n + divisor - 1) & ~(divisor - 1);
}

static uintptr_t
round_down_to(uintptr_t n, size_t divisor)
{
  asrt(is_power_of_two(divisor));
  return n & ~(divisor - 1);
}

#define CHUNK_ALIGN 16

#define FOOTER_SIZE (round_up_to(sizeof(chunkfooter_t), CHUNK_ALIGN))

#define DEFAULT_CHUNK_SIZE_WITHOUT_FOOTER (4096 - FOOTER_SIZE)

/*
 * --- 哨兵 (Sentinel) 空 Chunk ---
 */

static chunkfooter_t EMPTY_CHUNK_SINGLETON;
static bool EMPTY_CHUNK_INITIALIZED = false;

static chunkfooter_t *
get_empty_chunk()
{
  if (!EMPTY_CHUNK_INITIALIZED)
  {

    EMPTY_CHUNK_SINGLETON.data = (unsigned char *)&EMPTY_CHUNK_SINGLETON;
    EMPTY_CHUNK_SINGLETON.chunk_size = 0;
    EMPTY_CHUNK_SINGLETON.prev = &EMPTY_CHUNK_SINGLETON;
    EMPTY_CHUNK_SINGLETON.ptr = (unsigned char *)&EMPTY_CHUNK_SINGLETON;
    EMPTY_CHUNK_SINGLETON.allocated_bytes = 0;
    EMPTY_CHUNK_INITIALIZED = true;
  }
  return &EMPTY_CHUNK_SINGLETON;
}

static bool
chunk_is_empty(chunkfooter_t *footer)
{
  return footer == get_empty_chunk();
}

/*
 * --- 内部 Chunk 管理 ---
 */

static void
dealloc_chunk_list(chunkfooter_t *footer)
{
  while (!chunk_is_empty(footer))
  {
    chunkfooter_t *prev = footer->prev;

    aligned_free_internal(footer->data);
    footer = prev;
  }
}

static chunkfooter_t *
new_chunk(bump_t *bump, size_t new_size_without_footer, size_t align, chunkfooter_t *prev)
{

  new_size_without_footer = round_up_to(new_size_without_footer, CHUNK_ALIGN);

  size_t alloc_size;
  if (__builtin_add_overflow(new_size_without_footer, FOOTER_SIZE, &alloc_size))
  {
    return NULL;
  }

  alloc_size = round_up_to(alloc_size, align);
  if (alloc_size == 0)
    return NULL;

  unsigned char *data = (unsigned char *)aligned_malloc_internal(align, alloc_size);
  if (!data)
    return NULL;

  chunkfooter_t *footer_ptr = (chunkfooter_t *)(data + new_size_without_footer);

  footer_ptr->data = data;
  footer_ptr->chunk_size = alloc_size;
  footer_ptr->prev = prev;

  footer_ptr->allocated_bytes = prev->allocated_bytes + new_size_without_footer;

  uintptr_t ptr_start = (uintptr_t)footer_ptr;
  footer_ptr->ptr = (unsigned char *)round_down_to(ptr_start, bump->min_align);

  asrt(footer_ptr->ptr >= footer_ptr->data);

  return footer_ptr;
}

/*
 * --- 分配慢速路径 (Slow Path) ---
 */

static void *
alloc_layout_slow(bump_t *bump, layout_t layout)
{
  chunkfooter_t *current_footer = bump->current_chunk_footer;

  size_t prev_usable_size = 0;
  if (!chunk_is_empty(current_footer))
  {
    prev_usable_size = current_footer->chunk_size - FOOTER_SIZE;
  }

  size_t new_size_without_footer;
  if (__builtin_mul_overflow(prev_usable_size, 2, &new_size_without_footer))
  {
    new_size_without_footer = SIZE_MAX;
  }

  if (new_size_without_footer < DEFAULT_CHUNK_SIZE_WITHOUT_FOOTER)
  {
    new_size_without_footer = DEFAULT_CHUNK_SIZE_WITHOUT_FOOTER;
  }

  size_t requested_align = (layout.align > bump->min_align) ? layout.align : bump->min_align;
  size_t requested_size = round_up_to(layout.size, requested_align);

  if (new_size_without_footer < requested_size)
  {
    new_size_without_footer = requested_size;
  }

  if (bump->allocation_limit != SIZE_MAX)
  {
    size_t allocated = current_footer->allocated_bytes;
    size_t limit = bump->allocation_limit;
    size_t remaining = (limit > allocated) ? (limit - allocated) : 0;

    if (new_size_without_footer > remaining)
    {

      if (requested_size > remaining)
      {
        return NULL;
      }

      new_size_without_footer = requested_size;
    }
  }

  size_t chunk_align = (layout.align > CHUNK_ALIGN) ? layout.align : CHUNK_ALIGN;
  chunk_align = (chunk_align > bump->min_align) ? chunk_align : bump->min_align;

  chunkfooter_t *new_footer = new_chunk(bump, new_size_without_footer, chunk_align, current_footer);
  if (!new_footer)
  {
    return NULL;
  }

  bump->current_chunk_footer = new_footer;

  chunkfooter_t *footer = new_footer;
  unsigned char *ptr = footer->ptr;
  unsigned char *start = footer->data;

  unsigned char *result_ptr;
  size_t aligned_size;

  asrt(((uintptr_t)ptr % bump->min_align) == 0);

  if (layout.align <= bump->min_align)
  {

    if (__builtin_add_overflow(layout.size, bump->min_align - 1, &aligned_size))
      return NULL;
    aligned_size = aligned_size & ~(bump->min_align - 1);

    size_t capacity = (size_t)(ptr - start);
    asrt_msg(aligned_size <= capacity, "New chunk too small!");

    result_ptr = ptr - aligned_size;
  }
  else
  {

    if (__builtin_add_overflow(layout.size, layout.align - 1, &aligned_size))
      return NULL;
    aligned_size = aligned_size & ~(layout.align - 1);

    unsigned char *aligned_ptr_end = (unsigned char *)round_down_to((uintptr_t)ptr, layout.align);

    asrt_msg(aligned_ptr_end >= start, "New chunk alignment failed!");
    size_t capacity = (size_t)(aligned_ptr_end - start);
    asrt_msg(aligned_size <= capacity, "New chunk too small!");

    result_ptr = aligned_ptr_end - aligned_size;
  }

  asrt(((uintptr_t)result_ptr % layout.align) == 0);
  asrt(result_ptr >= start);

  footer->ptr = result_ptr;
  return (void *)result_ptr;
}

/*
 * --- 分配快速路径 (Fast Path) ---
 */

static void *
try_alloc_layout_fast(bump_t *bump, layout_t layout)
{
  chunkfooter_t *footer = bump->current_chunk_footer;
  unsigned char *ptr = footer->ptr;
  unsigned char *start = footer->data;
  size_t min_align = bump->min_align;

  unsigned char *result_ptr;
  size_t aligned_size;

  asrt_msg((chunk_is_empty(footer) || ((uintptr_t)ptr % min_align) == 0), "bump_t pointer invariant broken");

  if (layout.align <= min_align)
  {

    if (__builtin_add_overflow(layout.size, min_align - 1, &aligned_size))
      return NULL;
    aligned_size = aligned_size & ~(min_align - 1);

    size_t capacity = (size_t)(ptr - start);
    if (aligned_size > capacity)
      return NULL;

    result_ptr = ptr - aligned_size;
  }
  else
  {

    if (__builtin_add_overflow(layout.size, layout.align - 1, &aligned_size))
      return NULL;
    aligned_size = aligned_size & ~(layout.align - 1);

    unsigned char *aligned_ptr_end = (unsigned char *)round_down_to((uintptr_t)ptr, layout.align);

    if (aligned_ptr_end < start)
      return NULL;

    size_t capacity = (size_t)(aligned_ptr_end - start);
    if (aligned_size > capacity)
      return NULL;

    result_ptr = aligned_ptr_end - aligned_size;
  }

  footer->ptr = result_ptr;
  return (void *)result_ptr;
}

/*
 * ========================================
 * --- 公共 API 实现 ---
 * ========================================
 */

/*
 * --- 生命周期 ---
 */

void bump_init_with_min_align(bump_t *bump, size_t min_align)
{
  asrt_msg(bump != NULL, "bump_t pointer cannot be NULL");
  asrt_msg(is_power_of_two(min_align), "min_align must be a power of two");
  asrt_msg(min_align <= CHUNK_ALIGN, "min_align cannot be larger than CHUNK_ALIGN (16)");

  bump->current_chunk_footer = get_empty_chunk();
  bump->allocation_limit = SIZE_MAX;
  bump->min_align = min_align;
}

void bump_init(bump_t *bump)
{
  bump_init_with_min_align(bump, 1);
}

bump_t *
bump_new_with_min_align(size_t min_align)
{
  bump_t *bump = (bump_t *)malloc(sizeof(bump_t));
  if (!bump)
    return NULL;
  bump_init_with_min_align(bump, min_align);
  return bump;
}

bump_t *
bump_new(void)
{
  return bump_new_with_min_align(1);
}

void bump_destroy(bump_t *bump)
{
  if (bump)
  {
    dealloc_chunk_list(bump->current_chunk_footer);

    bump->current_chunk_footer = get_empty_chunk();
  }
}

void bump_free(bump_t *bump)
{
  if (bump)
  {
    bump_destroy(bump);
    free(bump);
  }
}

void bump_reset(bump_t *bump)
{
  chunkfooter_t *current_footer = bump->current_chunk_footer;
  if (chunk_is_empty(current_footer))
  {
    return;
  }

  dealloc_chunk_list(current_footer->prev);

  current_footer->prev = get_empty_chunk();

  uintptr_t ptr_start = (uintptr_t)current_footer;
  current_footer->ptr = (unsigned char *)round_down_to(ptr_start, bump->min_align);

  size_t usable_size = (size_t)((unsigned char *)current_footer - current_footer->data);
  current_footer->allocated_bytes = usable_size;
}

/*
 * --- 分配 API ---
 */

void *
bump_alloc_layout(bump_t *bump, layout_t layout)
{

  if (layout.size == 0)
  {

    uintptr_t ptr = (uintptr_t)bump->current_chunk_footer->ptr;
    return (void *)round_down_to(ptr, layout.align);
  }
  if (layout.align == 0 || !is_power_of_two(layout.align))
  {

    layout.align = 1;
  }

  void *alloc = try_alloc_layout_fast(bump, layout);
  if (alloc)
  {
    return alloc;
  }

  return alloc_layout_slow(bump, layout);
}

void *
bump_alloc(bump_t *bump, size_t size, size_t align)
{
  layout_t layout = {size, align};
  return bump_alloc_layout(bump, layout);
}

void *
bump_alloc_copy(bump_t *bump, const void *src, size_t size, size_t align)
{
  if (size == 0)
  {

    layout_t layout = {size, align};
    return bump_alloc_layout(bump, layout);
  }

  void *dest = bump_alloc(bump, size, align);
  if (dest && src)
  {
    memcpy(dest, src, size);
  }
  return dest;
}

char *
bump_alloc_str(bump_t *bump, const char *str)
{
  if (!str)
    return NULL;

  size_t len = strlen(str);
  size_t size_with_null = len + 1;

  char *dest = (char *)bump_alloc(bump, size_with_null, __alignof(char));
  if (dest)
  {
    memcpy(dest, str, size_with_null);
  }
  return dest;
}

void *
bump_realloc(bump_t *bump, void *old_ptr, size_t old_size, size_t new_size, size_t align)
{

  if (old_ptr == NULL)
  {
    return bump_alloc(bump, new_size, align);
  }

  if (new_size == 0)
  {

    return NULL;
  }

  void *new_ptr = bump_alloc(bump, new_size, align);
  if (new_ptr == NULL)
  {
    return NULL;
  }

  size_t copy_size = (old_size < new_size) ? old_size : new_size;
  if (copy_size > 0)
  {
    memcpy(new_ptr, old_ptr, copy_size);
  }

  return new_ptr;
}

void *
bump_alloc_layout_zeroed(bump_t *bump, layout_t layout)
{
  void *ptr = bump_alloc_layout(bump, layout);

  // 确保在 OOM (ptr == NULL) 时不执行 memset
  if (ptr && layout.size > 0)
  {
    memset(ptr, 0, layout.size);
  }
  return ptr;
}

/*
 * --- 容量和限制 ---
 */

void bump_set_allocation_limit(bump_t *bump, size_t limit)
{
  bump->allocation_limit = limit;
}

size_t
bump_get_allocated_bytes(bump_t *bump)
{
  return bump->current_chunk_footer->allocated_bytes;
}