#include <stdio.h>
#include <string.h>

// --- Fluf 依赖 ---
#include <core/msg/asrt.h>
#include <core/mem/allocer.h>      // 抽象 Allocer
#include <std/allocer/bump/bump.h> // 具体 Bump 实现
#include <std/allocer/bump/glue.h> // "胶水" (bump -> allocer)

// --- 我们要测试的模块 ---
#include <std/math/bitset.h> // 你的路径

/**
 * @brief 辅助函数: 创建一个 3-bitset 用于测试
 */
static void
setup_sets(allocer_t *alc, bitset_t *sA, bitset_t *sB, bitset_t *sDest)
{
  // 100 bits (测试 > 64)
  size_t num_bits = 100;

  bool ok;
  ok = bitset_init(sA, num_bits, alc);
  asrt_msg(ok, "init A failed");
  ok = bitset_init(sB, num_bits, alc);
  asrt_msg(ok, "init B failed");
  ok = bitset_init(sDest, num_bits, alc);
  asrt_msg(ok, "init Dest failed");

  // sA = { 0, 65, 99 }
  bitset_set(sA, 0);
  bitset_set(sA, 65);
  bitset_set(sA, 99);

  // sB = { 1, 65, 70 }
  bitset_set(sB, 1);
  bitset_set(sB, 65);
  bitset_set(sB, 70);
}

/**
 * 测试 1：基本操作 (Set, Clear, Test, Count)
 */
static void
test_set_clear_test(void)
{
  printf("--- Test: test_set_clear_test ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  bitset_t bs;
  bool ok = bitset_init(&bs, 100, &alc); // 100 bits
  asrt_msg(ok, "bitset_init failed");
  asrt_msg(bs.num_bits == 100, "num_bits mismatch");
  asrt_msg(bs.num_words == 2, "num_words mismatch (100/64)");

  // 1. Test `clear_all` (默认)
  asrt_msg(!bitset_test(&bs, 0), "Should be clear (0)");
  asrt_msg(!bitset_test(&bs, 65), "Should be clear (65)");

  // 2. Test `set`
  bitset_set(&bs, 0);  // word 0, bit 0
  bitset_set(&bs, 63); // word 0, bit 63
  bitset_set(&bs, 64); // word 1, bit 0
  bitset_set(&bs, 99); // word 1, bit 35

  asrt_msg(bitset_test(&bs, 0), "Set failed (0)");
  asrt_msg(bitset_test(&bs, 63), "Set failed (63)");
  asrt_msg(bitset_test(&bs, 64), "Set failed (64)");
  asrt_msg(bitset_test(&bs, 99), "Set failed (99)");
  asrt_msg(!bitset_test(&bs, 1), "Set testing wrong bit (1)");

  // 3. Test `count`
  asrt_msg(bitset_count(&bs) == 4, "Count failed (should be 4)");

  // 4. Test `clear`
  bitset_clear(&bs, 63);
  asrt_msg(!bitset_test(&bs, 63), "Clear failed (63)");
  asrt_msg(bitset_count(&bs) == 3, "Count failed (should be 3)");

  bitset_destroy(&bs);
  bump_destroy(&arena);
}

/**
 * 测试 2：集合运算 (Union, Intersect, Diff)
 */
static void
test_set_ops(void)
{
  printf("--- Test: test_set_ops ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  bitset_t sA, sB, sDest;
  setup_sets(&alc, &sA, &sB, &sDest);

  // sA = { 0, 65, 99 }
  // sB = { 1, 65, 70 }

  // 1. Union: { 0, 1, 65, 70, 99 } (5 bits)
  bitset_union(&sDest, &sA, &sB);
  asrt_msg(bitset_test(&sDest, 0), "Union failed (0)");
  asrt_msg(bitset_test(&sDest, 1), "Union failed (1)");
  asrt_msg(bitset_test(&sDest, 65), "Union failed (65)");
  asrt_msg(bitset_test(&sDest, 70), "Union failed (70)");
  asrt_msg(bitset_test(&sDest, 99), "Union failed (99)");
  asrt_msg(bitset_count(&sDest) == 5, "Union count failed (should be 5)");

  // 2. Intersect: { 65 } (1 bit)
  bitset_intersect(&sDest, &sA, &sB);
  asrt_msg(!bitset_test(&sDest, 0), "Intersect failed (0)");
  asrt_msg(!bitset_test(&sDest, 1), "Intersect failed (1)");
  asrt_msg(bitset_test(&sDest, 65), "Intersect failed (65)");
  asrt_msg(bitset_count(&sDest) == 1, "Intersect count failed (should be 1)");

  // 3. Difference: sA \ sB = { 0, 99 } (2 bits)
  bitset_difference(&sDest, &sA, &sB);
  asrt_msg(bitset_test(&sDest, 0), "Diff failed (0)");
  asrt_msg(!bitset_test(&sDest, 1), "Diff failed (1)");
  asrt_msg(!bitset_test(&sDest, 65), "Diff failed (65)");
  asrt_msg(bitset_test(&sDest, 99), "Diff failed (99)");
  asrt_msg(bitset_count(&sDest) == 2, "Diff count failed (should be 2)");

  bitset_destroy(&sA);
  bitset_destroy(&sB);
  bitset_destroy(&sDest);
  bump_destroy(&arena);
}

/**
 * 测试 3：`set_all` 和 `init_all` (掩码)
 */
static void
test_set_all(void)
{
  printf("--- Test: test_set_all ---\n");
  bump_t arena;
  bump_init(&arena);
  allocer_t alc = bump_to_allocer(&arena);

  // 100 bits
  bitset_t bs;
  bitset_init_all(&bs, 100, &alc);

  // 1. 验证 0-99
  asrt_msg(bitset_test(&bs, 0), "init_all failed (0)");
  asrt_msg(bitset_test(&bs, 63), "init_all failed (63)");
  asrt_msg(bitset_test(&bs, 64), "init_all failed (64)");
  asrt_msg(bitset_test(&bs, 99), "init_all failed (99)");

  // 2. 验证 100+ (不应该被设置，尽管它在同一个 word 中)
  // (我们不能 `bitset_test(100)` 因为 asrt 会触发)
  // 我们可以直接检查 word[1] (bits 64-127)
  // 100 bits = 64 (word 0) + 36 (word 1)
  // remaining_bits = 36
  // mask = (1 << 36) - 1
  uint64_t mask = ((uint64_t)1 << 36) - 1;
  asrt_msg(bs.words[1] == mask, "init_all mask is incorrect");
  asrt_msg(bitset_count(&bs) == 100, "init_all count failed");

  // 3. `clear_all`
  bitset_clear_all(&bs);
  asrt_msg(!bitset_test(&bs, 0), "clear_all failed (0)");
  asrt_msg(!bitset_test(&bs, 99), "clear_all failed (99)");
  asrt_msg(bitset_count(&bs) == 0, "clear_all count failed");

  bitset_destroy(&bs);
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

  printf("=== [fluf] Running tests for <std/math/bitset> ===\n");

  test_set_clear_test();
  test_set_ops();
  test_set_all();

  printf("==================================================\n");
  printf("✅ All <std/math/bitset> tests passed!\n");
  printf("==================================================\n");

  return 0;
}