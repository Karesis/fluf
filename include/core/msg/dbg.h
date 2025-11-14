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