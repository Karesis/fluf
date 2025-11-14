
#pragma once

#include <stdio.h>

#ifdef NDEBUG

#define dbg(fmt, ...) ((void)0)

#else

/**
 * @brief (API) 打印调试信息 (printf 风格)
 *
 * 打印 [DEBUG] [file:line] func(): <message>
 */
#define dbg(fmt, ...)                                                          \
  do {                                                                         \
    fprintf(stderr, "[DEBUG] [%s:%d] %s(): " fmt "\n", __FILE__, __LINE__,     \
            __func__, __VA_ARGS__);                                            \
  } while (0)

#endif