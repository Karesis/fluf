#include <stdio.h>
#include <string.h>

// --- Fluf 依赖 ---
#include <core/mem/allocer.h> // 抽象 Allocer
#include <core/msg/asrt.h>
#include <std/allocer/bump/bump.h> // 具体 Bump 实现
#include <std/allocer/bump/glue.h> // "胶水" (bump -> allocer)

// --- 我们要测试的模块 ---
#include <std/hashmap/strhashmap.h> // 你的路径

/**
 * 辅助宏，用于将字符串字面量作为 void* 传递。
 * 这在测试中很方便，我们知道我们存的是什么。
 */
#define VAL(s) ((void *)(s))

/**
 * 测试 1：基本操作 (New, Put, Get, Free)
 */
static void test_put_get(void) {
  printf("--- Test: test_put_get ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  // 1. 创建 (使用默认容量)
  strhashmap_t *map = strhashmap_new(&alc, 0);
  asrt_msg(map != NULL, "strhashmap_new failed");
  asrt_msg(strhashmap_count(map) == 0, "Initial count should be 0");

  // 2. 插入
  bool ok = strhashmap_put(map, "key1", VAL("value1"));
  asrt_msg(ok, "strhashmap_put failed (OOM?)");
  asrt_msg(strhashmap_count(map) == 1, "Count should be 1 after put");

  // 3. 获取
  void *val = strhashmap_get(map, "key1");
  // 【【【 修复 】】】 (val == VAL("value1")) -> (strcmp(...) == 0)
  asrt_msg(strcmp((const char *)val, "value1") == 0, "strhashmap_get failed");

  // 4. 销毁
  strhashmap_free(map);
  bump_destroy(&arena);
}

/**
 * 测试 2：更新、获取不存在的键、`get_ptr` 安全性
 */
static void test_update_and_safe_get(void) {
  printf("--- Test: test_update_and_safe_get ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);
  strhashmap_t *map = strhashmap_new(&alc, 0);

  // 1. 获取不存在的键 (普通 get)
  void *val = strhashmap_get(map, "no_such_key");
  asrt_msg(val == NULL, "get on non-existent key should return NULL");

  // 2. 插入
  strhashmap_put(map, "key", VAL("value1"));
  // 【【【 修复 】】】
  asrt_msg(strcmp((const char *)strhashmap_get(map, "key"), "value1") == 0,
           "Get failed after put");

  // 3. 更新
  strhashmap_put(map, "key", VAL("value2"));
  asrt_msg(strhashmap_count(map) == 1, "Count should remain 1 after update");
  // 【【【 修复 】】】
  asrt_msg(strcmp((const char *)strhashmap_get(map, "key"), "value2") == 0,
           "Value was not updated");

  // 4. 测试 `get_ptr` (安全 get)
  void *out_val = NULL;
  bool found = strhashmap_get_ptr(map, "key", &out_val);
  asrt_msg(found == true, "get_ptr should find existing key");
  // 【【【 修复 】】】
  asrt_msg(strcmp((const char *)out_val, "value2") == 0,
           "get_ptr returned wrong value");

  found = strhashmap_get_ptr(map, "no_such_key", &out_val);
  asrt_msg(found == false, "get_ptr should not find non-existent key");

  // 5. 测试存储 NULL
  strhashmap_put(map, "null_value", NULL);
  asrt_msg(strhashmap_count(map) == 2, "Count should be 2");

  // (这些 NULL 检查是正确的，不需要改)
  asrt_msg(strhashmap_get(map, "null_value") == NULL,
           "Get for NULL value failed");
  found = strhashmap_get_ptr(map, "null_value", &out_val);
  asrt_msg(found == true, "get_ptr should find NULL value");
  asrt_msg(out_val == NULL, "get_ptr should report NULL value");

  strhashmap_free(map);
  bump_destroy(&arena);
}

/**
 * 测试 3：删除 (Delete)
 */
static void test_delete(void) {
  printf("--- Test: test_delete ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);
  strhashmap_t *map = strhashmap_new(&alc, 0);

  strhashmap_put(map, "key1", VAL("val1"));
  strhashmap_put(map, "key2", VAL("val2"));
  asrt_msg(strhashmap_count(map) == 2, "Setup failed");

  // 1. 删除存在的键
  bool ok = strhashmap_delete(map, "key1");
  asrt_msg(ok == true, "delete on existing key should return true");
  asrt_msg(strhashmap_count(map) == 1, "Count should be 1 after delete");
  asrt_msg(strhashmap_get(map, "key1") == NULL, "Key 1 was not deleted");
  // 【【【 修复 】】】
  asrt_msg(strcmp((const char *)strhashmap_get(map, "key2"), "val2") == 0,
           "Key 2 was wrongly deleted");

  // 2. 删除不存在的键
  ok = strhashmap_delete(map, "key1"); // 再次删除
  asrt_msg(ok == false, "delete on non-existent key should return false");
  ok = strhashmap_delete(map, "no_such_key");
  asrt_msg(ok == false, "delete on non-existent key should return false");
  asrt_msg(strhashmap_count(map) == 1, "Count should still be 1");

  // 3. 删除后重新插入 (测试墓碑)
  strhashmap_put(map, "key1", VAL("new_val1"));
  asrt_msg(strhashmap_count(map) == 2, "Count should be 2 after re-insert");
  // 【【【 修复 】】】
  asrt_msg(strcmp((const char *)strhashmap_get(map, "key1"), "new_val1") == 0,
           "Re-insert failed");

  strhashmap_free(map);
  bump_destroy(&arena);
}

/**
 * 测试 4：扩容 (Growth / Resize)
 */
static void test_growth(void) {
  printf("--- Test: test_growth ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  strhashmap_t *map = strhashmap_new(&alc, 4);

  strhashmap_put(map, "key1", VAL("val1"));
  strhashmap_put(map, "key2", VAL("val2"));
  strhashmap_put(map, "key3", VAL("val3"));
  asrt_msg(strhashmap_count(map) == 3, "Count should be 3");

  // 2. 触发扩容
  strhashmap_put(map, "key4", VAL("val4"));
  asrt_msg(strhashmap_count(map) == 4, "Count should be 4");

  // 3. 检查所有数据是否仍然存在
  // 【【【 修复 】】】
  asrt_msg(strcmp((const char *)strhashmap_get(map, "key1"), "val1") == 0,
           "Resize lost key1");
  asrt_msg(strcmp((const char *)strhashmap_get(map, "key2"), "val2") == 0,
           "Resize lost key2");
  asrt_msg(strcmp((const char *)strhashmap_get(map, "key3"), "val3") == 0,
           "Resize lost key3");
  asrt_msg(strcmp((const char *)strhashmap_get(map, "key4"), "val4") == 0,
           "Resize lost key4");

  // 4. 插入更多数据
  strhashmap_put(map, "key5", VAL("val5"));
  asrt_msg(strhashmap_count(map) == 5, "Count should be 5");
  // 【【【 修复 】】】
  asrt_msg(strcmp((const char *)strhashmap_get(map, "key5"), "val5") == 0,
           "Put after resize failed");

  strhashmap_free(map);
  bump_destroy(&arena);
}

/**
 * 测试 5：`clear`
 */
static void test_clear(void) {
  printf("--- Test: test_clear ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);
  strhashmap_t *map = strhashmap_new(&alc, 0);

  strhashmap_put(map, "key1", VAL("val1"));
  strhashmap_put(map, "key2", VAL("val2"));
  asrt_msg(strhashmap_count(map) == 2, "Setup failed");

  // 1. 清除
  strhashmap_clear(map);
  asrt_msg(strhashmap_count(map) == 0, "Count should be 0 after clear");
  asrt_msg(strhashmap_get(map, "key1") == NULL,
           "Key 1 should be gone after clear");
  asrt_msg(strhashmap_get(map, "key2") == NULL,
           "Key 2 should be gone after clear");

  // 2. 测试可重用性
  strhashmap_put(map, "key3", VAL("val3"));
  asrt_msg(strhashmap_count(map) == 1, "Count should be 1 after re-put");
  // 【【【 修复 】】】
  asrt_msg(strcmp((const char *)strhashmap_get(map, "key3"), "val3") == 0,
           "Re-put failed");

  strhashmap_free(map);
  bump_destroy(&arena);
}

/**
 * 测试 6：迭代器
 */
static void test_iterator(void) {
  printf("--- Test: test_iterator ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  strhashmap_t *map = strhashmap_new(&alc, 0);
  strhashmap_put(map, "A", VAL("1"));
  strhashmap_put(map, "B", VAL("2"));
  strhashmap_put(map, "C", VAL("3"));

  // 删除 "B" 来制造一个墓碑 (测试迭代器是否能跳过它)
  strhashmap_delete(map, "B");

  strhashmap_iter_t iter;
  strhashmap_iter_init(&iter, map);

  const char *key;
  void *val;
  int count = 0;
  bool found_A = false;
  bool found_C = false;

  while (strhashmap_iter_next(&iter, &key, &val)) {
    count++;
    if (strcmp(key, "A") == 0) {
      asrt_msg(strcmp((char *)val, "1") == 0, "Value mismatch for A");
      found_A = true;
    } else if (strcmp(key, "C") == 0) {
      asrt_msg(strcmp((char *)val, "3") == 0, "Value mismatch for C");
      found_C = true;
    } else {
      asrt_msg(false, "Found unexpected key (B should be deleted)");
    }
  }

  asrt_msg(count == 2, "Iterator count mismatch");
  asrt_msg(found_A && found_C, "Did not iterate all items");

  strhashmap_free(map);
  bump_destroy(&arena);
}

/**
 * 测试运行器 (Test Runner)
 */
int main(void) {
#ifdef NDEBUG
  fprintf(stderr, "Error: Cannot run tests with NDEBUG defined. Recompile in "
                  "Debug mode.\n");
  return 1;
#endif

  printf("=== [fluf] Running tests for <std/strhashmap> ===\n");

  test_put_get();
  test_update_and_safe_get();
  test_delete();
  test_growth();
  test_clear();
  test_iterator();

  printf("===============================================\n");
  printf("✅ All <std/strhashmap> tests passed!\n");
  printf("===============================================\n");

  return 0;
}