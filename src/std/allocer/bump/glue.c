#include <std/allocer/bump/glue.h>
#include <std/allocer/bump/bump.h>
#include <core/mem/allocer.h>
#include <core/msg/asrt.h>
#include <string.h> // for memset

/*
 * ===================================================================
 * 1. "蹦床"函数 (Trampolines)
 *
 * 这些是 vtable 的具体实现。它们是 `static` 的，
 * 只是将 `allocer_t` 接口的调用 "蹦床" (trampoline)
 * 到 `bump_t` 的具体实现上。
 * ===================================================================
 */

static void *
bump_vtable_alloc(void *self, layout_t layout)
{
  // "self" 指针就是我们的 bump_t*
  // bump_alloc_layout 可能会 OOM 返回 NULL，这符合 vtable 的预期
  return bump_alloc_layout((bump_t *)self, layout);
}

static void
bump_vtable_free(void *self, void *ptr, layout_t layout)
{
  (void)self; // Bump Arena 不支持单独释放
  (void)ptr;
  (void)layout;
  // 什么也不做 (This is the point of an arena)
}

static void *
bump_vtable_realloc(
    void *self,
    void *ptr,
    layout_t old_layout,
    layout_t new_layout)
{
  // bump_realloc 只是 "alloc + copy"
  // 注意它需要 old_size，我们从 old_layout 中获取
  return bump_realloc(
      (bump_t *)self,
      ptr,
      old_layout.size,
      new_layout.size,
      new_layout.align);
}

static void *
bump_vtable_zalloc(void *self, layout_t layout)
{
  // 我们重用 bump_alloc_layout_zeroed (它在 bump.c 中)
  return bump_alloc_layout_zeroed((bump_t *)self, layout);
}

/*
 * ===================================================================
 * 2. V-Table 静态实例
 * ===================================================================
 */

/**
 * @brief 这是 `bump_t` 分配器的全局、静态、只读 vtable。
 * 所有 `bump_to_allocer` 创建的句柄都将指向这个 vtable。
 */
static const allocer_vtable_t BUMP_ALLOC_VTABLE = {
    .alloc = bump_vtable_alloc,
    .free = bump_vtable_free,
    .realloc = bump_vtable_realloc,
    .zalloc = bump_vtable_zalloc,
};

/*
 * ===================================================================
 * 3. 公共 "构造函数" API
 * ===================================================================
 */

allocer_t
bump_to_allocer(bump_t *bump)
{
  asrt_msg(bump != NULL, "bump_t* cannot be NULL");

  return (allocer_t){
      .self = (void *)bump,
      .vtable = &BUMP_ALLOC_VTABLE};
}