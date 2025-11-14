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

#pragma once

#include <stdio.h>
#include <stdlib.h>

#ifdef NDEBUG

#define asrt(cond) ((void)0)
#define asrt_msg(cond, fmt, ...) ((void)0)

#else

/**
 * @brief 简单断言 (效仿 C 标准 assert)
 */
#define asrt(cond)                                                             \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "Assertion Failed: (%s)\n  in %s() at %s:%d\n", #cond,   \
              __func__, __FILE__, __LINE__);                                   \
      abort();                                                                 \
    }                                                                          \
  } while (0)

/**
 * @brief 带自定义消息的断言 (使用 printf 风格)
 */
#define asrt_msg(cond, fmt, ...)                                               \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr,                                                          \
              "Assertion Failed: (%s)\n  in %s() at %s:%d\n  Message: " fmt    \
              "\n",                                                            \
              #cond, __func__, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__); \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#endif