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

#include <std/string/strslice.h>
#include <stdbool.h>
#include <string.h>

/**
 * @brief (枚举) 描述解析器返回的参数类型
 */
typedef enum {
  /** 没有更多参数了 */
  ARG_TYPE_END,
  /** 一个 "Flag" (以 '-' 开头), e.g., "-v", "-o", "--version" */
  ARG_TYPE_FLAG,
  /** 一个 "Positional" (不以 '-' 开头), e.g., "file.nyan" */
  ARG_TYPE_POSITIONAL
} arg_type_t;

/**
 * @brief (结构体) 参数解析器的状态
 * (在栈上初始化)
 */
typedef struct args_parser {
  int argc;
  const char **argv;
  int index;
} args_parser_t;

/**
 * @brief 初始化一个参数解析器 (在栈上)。
 *
 * @param p 指向要初始化的 args_parser_t 实例的指针。
 * @param argc `main` 函数的 argc。
 * @param argv `main` 函数的 argv。
 */
static inline void args_parser_init(args_parser_t *p, int argc,
                                    const char **argv) {
  p->argc = argc;
  p->argv = argv;
  p->index = 1;
}

/**
 * @brief (内部) 检查下一个参数 *而不* 消耗它。
 *
 * @param p The parser.
 * @param out_slice [out] 用于存储参数的 `strslice_t` 视图。
 * @return The type of the argument (FLAG, POSITIONAL, or END).
 */
static inline arg_type_t args_parser_peek(args_parser_t *p,
                                          strslice_t *out_slice) {
  if (p->index >= p->argc) {
    return ARG_TYPE_END;
  }

  const char *s = p->argv[p->index];
  *out_slice = slice_from_cstr(s);

  if (strcmp(s, "--") == 0) {
    return ARG_TYPE_END;
  }

  if (strcmp(s, "-") == 0) {
    return ARG_TYPE_POSITIONAL;
  }

  if (s[0] == '-') {
    return ARG_TYPE_FLAG;
  }

  return ARG_TYPE_POSITIONAL;
}

/**
 * @brief 消耗并返回下一个参数。
 *
 * @param p The parser.
 * @param out_slice [out] 用于存储参数的 `strslice_t` 视图。
 * @return The type of the argument (FLAG, POSITIONAL, or END).
 */
static inline arg_type_t args_parser_consume(args_parser_t *p,
                                             strslice_t *out_slice) {
  arg_type_t type = args_parser_peek(p, out_slice);

  if (type == ARG_TYPE_END) {

    if (p->index < p->argc && strcmp(p->argv[p->index], "--") == 0) {
      p->index++;
    }
    return ARG_TYPE_END;
  }

  p->index++;
  return type;
}

/**
 * @brief (便捷函数) 消耗下一个参数，并期望它是一个值。
 * * 用于解析 "-o <value>"
 *
 * @param p The parser.
 * @param option_name 用于错误报告的选项名称 (e.g., "-o")
 * @param out_value [out] 存储值
 * @return true (成功) 或 false (缺失值)
 */
static inline bool args_parser_consume_value(args_parser_t *p,
                                             const char *option_name,
                                             strslice_t *out_value) {
  arg_type_t type = args_parser_consume(p, out_value);

  if (type != ARG_TYPE_POSITIONAL) {

    return false;
  }

  return true;
}