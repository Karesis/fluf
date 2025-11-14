#pragma once

#include <core/mem/allocer.h>
#include <core/span.h>
#include <std/string/str_slice.h>
#include <std/vec.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief (语义别名) 在诊断上下文中, span_t 被称为 source_span_t
 */
typedef span_t source_span_t;

/**
 * @brief (数据) 源码管理器查找的结果
 */
typedef struct source_loc {
  const char *filename;
  size_t line;
  size_t column;
} source_loc_t;

/**
 * @brief 源码管理器
 *
 * `sourcemap_t` 是一个有状态的对象，它在栈上分配，
 * 并使用 `allocer_t` 来管理其*内部*的动态内存。
 */
typedef struct sourcemap {
  allocer_t *alc;
  vec_t files;
  size_t total_size;
} sourcemap_t;
/**
 * @brief 初始化一个 sourcemap (例如在栈上)
 *
 * @param map 指向要初始化的 sourcemap 实例的指针
 * @param alc 用于所有内部存储的分配器 (vtable 句柄)
 * @return true (成功) 或 false (OOM)
 */
bool sourcemap_init(sourcemap_t *map, allocer_t *alc);

/**
 * @brief 销毁 sourcemap 的内部存储
 */
void sourcemap_destroy(sourcemap_t *map);

/**
 * @brief (核心) 向 sourcemap 添加一个源文件
 *
 * 这会扫描文件中的所有换行符 ('\n') 以构建行查找表。
 * 它会*复制* `filename`，但*只存储* `slice` 的视图。
 *
 * @param map 源码管理器
 * @param filename 此文件的名称 (例如 "main.nyan")。
 * @param slice 从 `read_file_to_slice` 获得的源文件内容。
 * @return 一个 "FileID" (文件索引)，如果失败则返回 (size_t)-1
 */
size_t sourcemap_add_file(sourcemap_t *map, const char *filename,
                          str_slice_t slice);

/**
 * @brief (核心) 将全局字节偏移量转换为 "行:列"
 *
 * @param map 源码管理器
 * @param offset *全局*字节偏移量
 * @param out_loc [out] 成功时，写入结果
 * @return true (成功) 或 false (偏移量越界)
 */
bool sourcemap_lookup(const sourcemap_t *map, size_t offset,
                      source_loc_t *out_loc);

/**
 * @brief (辅助) 将一个 `source_span_t` 转换为起始位置
 */
static inline bool sourcemap_lookup_span(const sourcemap_t *map,
                                         source_span_t span,
                                         source_loc_t *out_loc) {
  return sourcemap_lookup(map, span.start, out_loc);
}