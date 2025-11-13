#include <stdio.h>
#include <string.h>

// --- Fluf 依赖 ---
#include <core/msg/asrt.h>
#include <core/mem/allocer.h>      // 抽象 Allocer
#include <std/allocer/bump/bump.h> // 具体 Bump 实现
#include <std/allocer/bump/glue.h> // "胶水" (bump -> allocer)

// --- 我们要测试的模块 ---
#include <std/vec.h> // 你的路径

/**
 * 测试 1：基本操作 (Init, Push, Get, Destroy)
 */
static void
test_push_get(void)
{
  printf("--- Test: test_push_get ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  // 1. 初始化
  vec_t vec;
  bool ok = vec_init(&vec, &alc, 0); // 0 = 使用默认容量
  asrt_msg(ok, "vec_init failed (OOM?)");
  asrt_msg(vec_count(&vec) == 0, "Initial count should be 0");

  // 2. 插入 (使用 (void*) 转换整数作为测试值)
  ok = vec_push(&vec, (void *)(uintptr_t)100);
  asrt_msg(ok, "vec_push failed (OOM?)");
  ok = vec_push(&vec, (void *)(uintptr_t)200);
  asrt_msg(ok, "vec_push failed (OOM?)");

  asrt_msg(vec_count(&vec) == 2, "Count should be 2 after 2 pushes");

  // 3. 获取
  void *val1 = vec_get(&vec, 0);
  void *val2 = vec_get(&vec, 1);
  asrt_msg(val1 == (void *)(uintptr_t)100, "vec_get(0) returned wrong value");
  asrt_msg(val2 == (void *)(uintptr_t)200, "vec_get(1) returned wrong value");

  // 4. 销毁
  vec_destroy(&vec);
  bump_destroy(&arena);
}

/**
 * 测试 2：Pop 和 Set
 */
static void
test_pop_set(void)
{
  printf("--- Test: test_pop_set ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);
  vec_t vec;
  vec_init(&vec, &alc, 0);

  vec_push(&vec, (void *)(uintptr_t)1);
  vec_push(&vec, (void *)(uintptr_t)2);
  vec_push(&vec, (void *)(uintptr_t)3);
  asrt_msg(vec_count(&vec) == 3, "Setup failed");

  // 1. Pop
  void *val = vec_pop(&vec);
  asrt_msg(val == (void *)(uintptr_t)3, "pop returned wrong value");
  asrt_msg(vec_count(&vec) == 2, "Count should be 2 after pop");

  val = vec_pop(&vec);
  asrt_msg(val == (void *)(uintptr_t)2, "pop (2) returned wrong value");
  asrt_msg(vec_count(&vec) == 1, "Count should be 1 after pop (2)");

  // 2. Set (覆盖)
  vec_set(&vec, 0, (void *)(uintptr_t)99);
  asrt_msg(vec_count(&vec) == 1, "Count should be 1 after set");
  asrt_msg(vec_get(&vec, 0) == (void *)(uintptr_t)99, "vec_set failed to update");

  // 3. Pop 最后一个
  val = vec_pop(&vec);
  asrt_msg(val == (void *)(uintptr_t)99, "pop (3) returned wrong value");
  asrt_msg(vec_count(&vec) == 0, "Count should be 0 after final pop");

  // 4. Pop 空 vector
  val = vec_pop(&vec);
  asrt_msg(val == NULL, "pop on empty vector should return NULL");

  vec_destroy(&vec);
  bump_destroy(&arena);
}

/**
 * 测试 3：扩容 (Growth / Resize)
 */
static void
test_growth(void)
{
  printf("--- Test: test_growth ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  // 1. 初始化一个小容量 (2) 来强制扩容
  vec_t vec;
  vec_init(&vec, &alc, 2);
  asrt_msg(vec.capacity == 2, "Initial capacity was not 2");

  vec_push(&vec, (void *)(uintptr_t)1);
  vec_push(&vec, (void *)(uintptr_t)2);
  asrt_msg(vec_count(&vec) == 2, "Count should be 2 (full)");

  // 2. 触发扩容
  vec_push(&vec, (void *)(uintptr_t)3);
  asrt_msg(vec_count(&vec) == 3, "Count should be 3 (after resize)");
  asrt_msg(vec.capacity >= 3, "Capacity did not increase"); // 应该是 4

  // 3. 检查所有数据是否仍然存在
  asrt_msg(vec_get(&vec, 0) == (void *)(uintptr_t)1, "Resize lost val 1");
  asrt_msg(vec_get(&vec, 1) == (void *)(uintptr_t)2, "Resize lost val 2");
  asrt_msg(vec_get(&vec, 2) == (void *)(uintptr_t)3, "Resize lost val 3");

  // 4. 继续插入
  vec_push(&vec, (void *)(uintptr_t)4);
  vec_push(&vec, (void *)(uintptr_t)5); // 再次触发扩容
  asrt_msg(vec_count(&vec) == 5, "Count should be 5");
  asrt_msg(vec.capacity >= 5, "Capacity did not increase (2nd)");
  asrt_msg(vec_get(&vec, 4) == (void *)(uintptr_t)5, "Resize lost val 5");

  vec_destroy(&vec);
  bump_destroy(&arena);
}

/**
 * 测试 4：边界条件 (Clear, Bounds)
 */
static void
test_boundaries(void)
{
  printf("--- Test: test_boundaries ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);
  vec_t vec;
  vec_init(&vec, &alc, 0);

  vec_push(&vec, (void *)(uintptr_t)1);
  vec_push(&vec, (void *)(uintptr_t)2);
  asrt_msg(vec_count(&vec) == 2, "Setup failed");

  // 1. `clear`
  vec_clear(&vec);
  asrt_msg(vec_count(&vec) == 0, "Count should be 0 after clear");
  asrt_msg(vec.capacity != 0, "Capacity should not be 0 after clear");

  // 2. `get` 越界
  void *val = vec_get(&vec, 0);
  asrt_msg(val == NULL, "get on empty vector should be NULL");

  // 3. 在 clear 后重用
  vec_push(&vec, (void *)(uintptr_t)3);
  asrt_msg(vec_count(&vec) == 1, "Count should be 1 after re-push");
  asrt_msg(vec_get(&vec, 0) == (void *)(uintptr_t)3, "Get after clear failed");

  // 4. `get` 越界 (非空)
  val = vec_get(&vec, 1); // 索引 1 越界 (count=1)
  asrt_msg(val == NULL, "get out-of-bounds should be NULL");
  val = vec_get(&vec, 99);
  asrt_msg(val == NULL, "get way-out-of-bounds should be NULL");

  vec_destroy(&vec);
  bump_destroy(&arena);
}

/**
 * 测试运行器 (Test Runner)
 */
int main(void)
{
#ifdef NDEBUG
  fprintf(stderr, "Error: Cannot run tests with NDEBUG defined. Recompile in Debug mode.\n");
  return 1;
#endif

  printf("=== [fluf] Running tests for <std/vec> ===\n");

  test_push_get();
  test_pop_set();
  test_growth();
  test_boundaries();

  printf("==========================================\n");
  printf("✅ All <std/vec> tests passed!\n");
  printf("==========================================\n");

  return 0;
}