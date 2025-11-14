# std/env/args.h

## `typedef enum {`


(枚举) 描述解析器返回的参数类型


---

## `ARG_TYPE_POSITIONAL } arg_type_t;`

一个 "Positional" (不以 '-' 开头), e.g., "file.nyan" 

---

## `typedef struct args_parser {`


(结构体) 参数解析器的状态
(在栈上初始化)


---

## `static inline void args_parser_init(args_parser_t *p, int argc, const char **argv) {`


初始化一个参数解析器 (在栈上)。


- **`p`**: 指向要初始化的 args_parser_t 实例的指针。
- **`argc`**: `main` 函数的 argc。
- **`argv`**: `main` 函数的 argv。


---

## `static inline arg_type_t args_parser_peek(args_parser_t *p, str_slice_t *out_slice) {`


(内部) 检查下一个参数 *而不* 消耗它。


- **`p`**: The parser.
- **`out_slice`**: [out] 用于存储参数的 `str_slice_t` 视图。
- **Returns**: The type of the argument (FLAG, POSITIONAL, or END).


---

## `static inline arg_type_t args_parser_consume(args_parser_t *p, str_slice_t *out_slice) {`


消耗并返回下一个参数。


- **`p`**: The parser.
- **`out_slice`**: [out] 用于存储参数的 `str_slice_t` 视图。
- **Returns**: The type of the argument (FLAG, POSITIONAL, or END).


---

## `static inline bool args_parser_consume_value(args_parser_t *p, const char *option_name, str_slice_t *out_value) {`


(便捷函数) 消耗下一个参数，并期望它是一个值。
* 用于解析 "-o <value>"


- **`p`**: The parser.
- **`option_name`**: 用于错误报告的选项名称 (e.g., "-o")
- **`out_value`**: [out] 存储值
- **Returns**: true (成功) 或 false (缺失值)


---

