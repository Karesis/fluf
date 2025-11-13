// include/core/dbg.h
#pragma once

#include <stdio.h> // for fprintf, stderr

// dbg 宏也应该遵循 NDEBUG 规范
#ifdef NDEBUG

// 在发布模式 (Release) 下，`dbg` 什么也不做
#define dbg(fmt, ...) ((void)0)

#else

// 在调试模式 (Debug) 下，打印调试信息
/**
 * @brief (API) 打印调试信息 (printf 风格)
 *
 * 打印 [DEBUG] [file:line] func(): <message>
 */
#define dbg(fmt, ...)                                   \
  do                                                    \
  {                                                     \
    fprintf(stderr,                                     \
            "[DEBUG] [%s:%d] %s(): " fmt "\n",          \
            __FILE__, __LINE__, __func__, __VA_ARGS__); \
  } while (0)

#endif // NDEBUG