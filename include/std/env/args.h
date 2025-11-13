#pragma once

#include <std/string/str_slice.h> // 依赖 str_slice
#include <stdbool.h>
#include <string.h> // 依赖 strcmp

/**
 * @brief (枚举) 描述解析器返回的参数类型
 */
typedef enum
{
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
typedef struct args_parser
{
  int argc;
  const char **argv;
  int index; // 当前在 argv 中的位置
} args_parser_t;

/**
 * @brief 初始化一个参数解析器 (在栈上)。
 *
 * @param p 指向要初始化的 args_parser_t 实例的指针。
 * @param argc `main` 函数的 argc。
 * @param argv `main` 函数的 argv。
 */
static inline void
args_parser_init(args_parser_t *p, int argc, const char **argv)
{
  p->argc = argc;
  p->argv = argv;
  p->index = 1; // 自动跳过程序名称 (argv[0])
}

/**
 * @brief (内部) 检查下一个参数 *而不* 消耗它。
 *
 * @param p The parser.
 * @param out_slice [out] 用于存储参数的 `str_slice_t` 视图。
 * @return The type of the argument (FLAG, POSITIONAL, or END).
 */
static inline arg_type_t
args_parser_peek(args_parser_t *p, str_slice_t *out_slice)
{
  if (p->index >= p->argc)
  {
    return ARG_TYPE_END;
  }

  const char *s = p->argv[p->index];
  *out_slice = slice_from_cstr(s);

  // 特殊情况 1: "--" (end of flags)
  // 把它当作 END，这样循环就会停止
  if (strcmp(s, "--") == 0)
  {
    return ARG_TYPE_END;
  }

  // 特殊情况 2: "-" (stdin)
  // 把它当作一个*位置参数*，而不是 flag
  if (strcmp(s, "-") == 0)
  {
    return ARG_TYPE_POSITIONAL;
  }

  // 常规情况
  if (s[0] == '-')
  {
    return ARG_TYPE_FLAG;
  }

  return ARG_TYPE_POSITIONAL;
}

/**
 * @brief 消耗并返回下一个参数。
 *
 * @param p The parser.
 * @param out_slice [out] 用于存储参数的 `str_slice_t` 视图。
 * @return The type of the argument (FLAG, POSITIONAL, or END).
 */
static inline arg_type_t
args_parser_consume(args_parser_t *p, str_slice_t *out_slice)
{
  arg_type_t type = args_parser_peek(p, out_slice);

  if (type == ARG_TYPE_END)
  {
    // 如果我们是因 "--" 而停止，
    // 我们需要消耗它，这样
    // 后续的位置参数才能被正确解析。
    if (p->index < p->argc && strcmp(p->argv[p->index], "--") == 0)
    {
      p->index++;
    }
    return ARG_TYPE_END;
  }

  // 消耗
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
static inline bool
args_parser_consume_value(args_parser_t *p, const char *option_name, str_slice_t *out_value)
{
  arg_type_t type = args_parser_consume(p, out_value);

  // 如果下一个是 END 或 另一个 Flag，则说明缺失值
  if (type != ARG_TYPE_POSITIONAL)
  {
    // (可选: 打印错误)
    // fprintf(stderr, "Error: Missing value for %s\n", option_name);
    return false;
  }

  return true;
}