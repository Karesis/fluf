#include <stdio.h>
#include <string.h>
#include <stdint.h> // for uintptr_t

// 包含我们要测试的模块
#include <std/allocer/bump/bump.h>

// 包含我们的测试工具
#include <core/msg/asrt.h>
#include <core/span.h>
#include <core/msg/dbg.h> // 可选，用于打印信息

/**
 * 测试 1：栈上 Arena 的生命周期
 * (bump_init / bump_destroy)
 */
static void
test_lifecycle_stack(void)
{
  printf("--- Test: test_lifecycle_stack ---\n");
  bump_t arena;
  bump_init(&arena);

  // 检查初始状态
  asrt_msg(bump_get_allocated_bytes(&arena) == 0, "Initial allocated bytes should be 0");

  // 分配一点内存
  void *ptr = BUMP_ALLOC(&arena, int);
  asrt_msg(ptr != NULL, "Stack arena failed to alloc");
  asrt_msg(bump_get_allocated_bytes(&arena) > 0, "Allocated bytes should be > 0 after alloc");

  // 销毁
  bump_destroy(&arena);

  // 检查销毁后的状态 (公共 API)
  asrt_msg(bump_get_allocated_bytes(&arena) == 0, "Allocated bytes should be 0 after destroy");
}

/**
 * 测试 2：堆上 Arena 的生命周期
 * (bump_new / bump_free)
 */
static void
test_lifecycle_heap(void)
{
  printf("--- Test: test_lifecycle_heap ---\n");
  bump_t *arena = bump_new();
  asrt_msg(arena != NULL, "bump_new failed (OOM?)");

  void *ptr = BUMP_ALLOC(arena, int);
  asrt_msg(ptr != NULL, "Heap arena failed to alloc");

  bump_free(arena);
}

/**
 * 测试 3：对齐 (Alignment) - 关键测试!
 */
static void
test_alignment(void)
{
  printf("--- Test: test_alignment ---\n");
  bump_t arena;
  bump_init(&arena);

  // 分配不同对齐的类型
  char *p1 = BUMP_ALLOC(&arena, char);     // alignof(char) = 1
  int *p2 = BUMP_ALLOC(&arena, int);       // alignof(int) = 4
  double *p3 = BUMP_ALLOC(&arena, double); // alignof(double) = 8

  asrt_msg(p1 && p2 && p3, "Alignment test failed to alloc");

  // 检查指针地址的对齐
  uintptr_t addr1 = (uintptr_t)p1;
  uintptr_t addr2 = (uintptr_t)p2;
  uintptr_t addr3 = (uintptr_t)p3;

  // dbg("Addr char: %p, int: %p, double: %p", p1, p2, p3);

  asrt_msg((addr1 % __alignof(char)) == 0, "char alignment failed");
  asrt_msg((addr2 % __alignof(int)) == 0, "int alignment failed");
  asrt_msg((addr3 % __alignof(double)) == 0, "double alignment failed");

  bump_destroy(&arena);
}

/**
 * 测试 4：清零分配 (Zeroed Allocation)
 */
static void
test_zeroed(void)
{
  printf("--- Test: test_zeroed ---\n");
  bump_t arena;
  bump_init(&arena);

  // 定义一个结构体
  typedef struct
  {
    int x;
    int y;
  } Point;

  Point *p = BUMP_ALLOC_ZEROED(&arena, Point);
  asrt_msg(p != NULL, "zeroed alloc failed");

  // 检查内存是否真的为 0
  asrt_msg(p->x == 0 && p->y == 0, "BUMP_ALLOC_ZEROED failed to clear memory");

  // 检查 Slice 版本
  int *arr = BUMP_ALLOC_SLICE_ZEROED(&arena, int, 10);
  asrt_msg(arr != NULL, "zeroed slice alloc failed");
  for_range(i, 0, 10)
  {
    asrt_msg(arr[i] == 0, "Slice element was not zero");
  }

  bump_destroy(&arena);
}

/**
 * 测试 5：字符串分配
 */
static void
test_str(void)
{
  printf("--- Test: test_str ---\n");
  bump_t arena;
  bump_init(&arena);

  const char *original = "Hello Fluf!";
  char *copied = bump_alloc_str(&arena, original);

  asrt_msg(copied != NULL, "bump_alloc_str failed");
  asrt_msg(copied != original, "String was not copied (pointers are same)");
  asrt_msg(strcmp(copied, original) == 0, "String content does not match");

  bump_destroy(&arena);
}

/**
 * 测试 6：增长（慢速路径）
 * 强制分配器创建新的 Chunk
 */
static void
test_growth(void)
{
  printf("--- Test: test_growth ---\n");
  bump_t arena;
  bump_init(&arena);

  // 默认 Chunk 大约 4KB (4096 - footer)
  // 我们分配 5KB 来强制触发 slow_path 和 new_chunk

  size_t size = 5000;
  void *ptr = bump_alloc(&arena, size, 1);
  asrt_msg(ptr != NULL, "Growth alloc failed");

  // 检查 arena 现在是否不再是空的或单个 chunk (取决于实现细节)
  // 一个简单的检查是：总分配字节数是否 > 0
  asrt_msg(bump_get_allocated_bytes(&arena) > 0, "Allocated bytes not updated");

  // 再次分配，确保 arena 仍然可用
  int *p2 = BUMP_ALLOC(&arena, int);
  asrt_msg(p2 != NULL, "Alloc after growth failed");

  bump_destroy(&arena);
}

/**
 * 测试运行器 (Test Runner)
 */
int main(void)
{
  // 如果 NDEBUG 被定义了，我们的断言将不起作用
#ifdef NDEBUG
  fprintf(stderr, "Error: Cannot run tests with NDEBUG defined. Recompile in Debug mode.\n");
  return 1;
#endif

  printf("=== [fluf] Running tests for <std/bump> ===\n");

  test_lifecycle_stack();
  test_lifecycle_heap();
  test_alignment();
  test_zeroed();
  test_str();
  test_growth();
  // TODO: 添加 test_reset()
  // TODO: 添加 test_realloc()
  // TODO: 添加 test_oom_limit()

  printf("===========================================\n");
  printf("✅ All <std/bump> tests passed!\n");
  printf("===========================================\n");

  return 0;
}