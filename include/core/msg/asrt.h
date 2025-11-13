#pragma once

#include <stdio.h>  // for fprintf, stderr
#include <stdlib.h> // for abort

// 和 <assert.h> 一样, 遵循 NDEBUG 规范
#ifdef NDEBUG

// 在发布模式 (Release) 下，断言是“零开销”的，它们什么也不做
#define asrt(cond) ((void)0)
#define asrt_msg(cond, fmt, ...) ((void)0)

#else

// 在调试模式 (Debug) 下，我们才检查

/**
 * @brief 简单断言 (效仿 C 标准 assert)
 */
#define asrt(cond)                                            \
  do                                                          \
  {                                                           \
    if (!(cond))                                              \
    {                                                         \
      fprintf(stderr,                                         \
              "Assertion Failed: (%s)\n  in %s() at %s:%d\n", \
              #cond, __func__, __FILE__, __LINE__);           \
      abort();                                                \
    }                                                         \
  } while (0)

/**
 * @brief 带自定义消息的断言 (使用 printf 风格)
 */
#define asrt_msg(cond, fmt, ...)                                                  \
  do                                                                              \
  {                                                                               \
    if (!(cond))                                                                  \
    {                                                                             \
      fprintf(stderr,                                                             \
              "Assertion Failed: (%s)\n  in %s() at %s:%d\n  Message: " fmt "\n", \
              #cond, __func__, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__);    \
      abort();                                                                    \
    }                                                                             \
  } while (0)

#endif