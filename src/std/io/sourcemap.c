#include <std/io/sourcemap.h>
#include <std/vec.h>         // 依赖 Vec<void*>
#include <core/mem/layout.h> // 依赖 layout_of
#include <core/msg/asrt.h>   // 依赖 asrt_msg
#include <stdint.h>
#include <string.h> // 依赖 strlen, memcpy

// --- 内部结构 ---

/**
 * @brief 存储单个已加载文件及其行查找表
 */
typedef struct
{
  const char *filename; // 拥有的副本 (由 alc 分配)
  str_slice_t content;  // 非拥有的视图 (指向 Arena)
  vec_t line_starts;    // Vec<void*> (存储 line_start_offset)
  size_t base_offset;   // 此文件在*全局*偏移量中的起始点
  size_t length;        // content.len
} SourceFile;

// --- 公共 API 实现 ---

bool sourcemap_init(sourcemap_t *map, allocer_t *alc)
{
  asrt_msg(map != NULL, "Sourcemap pointer cannot be NULL");
  asrt_msg(alc != NULL, "Allocator cannot be NULL");

  map->alc = alc;
  map->total_size = 0;

  // 初始化外层 vector (用于存储 SourceFile*)
  return vec_init(&map->files, alc, 0);
}

void sourcemap_destroy(sourcemap_t *map)
{
  if (!map)
    return;

  // 循环遍历并销毁所有 SourceFile 对象
  for (size_t i = 0; i < vec_count(&map->files); i++)
  {
    SourceFile *file = vec_get(&map->files, i);
    if (file)
    {
      // 1. 释放 `filename` (我们复制了它)
      allocer_free(map->alc, (void *)file->filename, layout_of_array(char, strlen(file->filename) + 1));

      // 2. 销毁 `line_starts` (内部 vector)
      vec_destroy(&file->line_starts);

      // 3. 释放 `SourceFile` 结构体本身
      allocer_free(map->alc, file, layout_of(SourceFile));
    }
  }

  // 销毁外层 vector
  vec_destroy(&map->files);
  memset(map, 0, sizeof(sourcemap_t)); // 清理
}

size_t
sourcemap_add_file(sourcemap_t *map, const char *filename, str_slice_t slice)
{
  // 1. 为 SourceFile 结构体分配内存
  SourceFile *file = allocer_alloc(map->alc, layout_of(SourceFile));
  if (!file)
    return (size_t)-1; // OOM

  // 2. 复制 filename
  size_t filename_len = strlen(filename);
  layout_t name_layout = layout_of_array(char, filename_len + 1);
  char *filename_copy = allocer_alloc(map->alc, name_layout);
  if (!filename_copy)
  {
    allocer_free(map->alc, file, layout_of(SourceFile));
    return (size_t)-1; // OOM
  }
  memcpy(filename_copy, filename, filename_len + 1);

  // 3. 初始化 `line_starts` vector
  if (!vec_init(&file->line_starts, map->alc, 0))
  {
    allocer_free(map->alc, filename_copy, name_layout);
    allocer_free(map->alc, file, layout_of(SourceFile));
    return (size_t)-1; // OOM
  }

  // 4. 填充 file 结构体
  file->filename = filename_copy;
  file->content = slice;
  file->length = slice.len;
  file->base_offset = map->total_size;

  // 5. 扫描换行符

  // Line 1 总是从 0 开始
  vec_push(&file->line_starts, (void *)(uintptr_t)0);

  for (size_t i = 0; i < slice.len; i++)
  {
    if (slice.ptr[i] == '\n')
    {
      // 下一行的起始偏移量是 \n 之后
      vec_push(&file->line_starts, (void *)(uintptr_t)(i + 1));
    }
  }

  // 6. 更新 sourcemap 状态
  vec_push(&map->files, file);
  map->total_size += file->length;

  // FileID 就是它在 `files` vector 中的索引
  return vec_count(&map->files) - 1;
}

/**
 * @brief (内部) 在 `line_starts` (一个 `vec_t`) 中二分查找
 * * 找到 `offset` 所在的行索引
 */
static size_t
find_line_index(const vec_t *lines, size_t offset)
{
  size_t low = 0;
  size_t high = vec_count(lines);
  size_t line_index = 0; // 默认为第一行

  // 二分查找：找到最后一个 `line_start <= offset` 的索引
  while (low < high)
  {
    size_t mid = low + (high - low) / 2;
    size_t mid_offset = (uintptr_t)vec_get(lines, mid);

    if (mid_offset > offset)
    {
      // 目标在左侧
      high = mid;
    }
    else
    {
      // 目标在右侧 (或就是 mid)
      line_index = mid; // 这是一个可能的答案
      low = mid + 1;
    }
  }
  return line_index;
}

bool sourcemap_lookup(const sourcemap_t *map, size_t offset, source_loc_t *out_loc)
{
  if (offset >= map->total_size)
  {
    return false; // 全局偏移量越界
  }

  // 1. 找到正确的文件 (线性查找，因为文件通常不多)
  SourceFile *file = NULL;
  for (size_t i = 0; i < vec_count(&map->files); i++)
  {
    SourceFile *f = vec_get(&map->files, i);
    if (offset >= f->base_offset && offset < f->base_offset + f->length)
    {
      file = f;
      break;
    }
  }

  if (file == NULL)
  {
    // (理论上不应该到这里，除非 total_size 计算有误)
    return false;
  }

  // 2. 转换为本地偏移量
  size_t local_offset = offset - file->base_offset;

  // 3. 在文件的 `line_starts` 中查找行
  size_t line_index = find_line_index(&file->line_starts, local_offset);
  size_t line_start_offset = (uintptr_t)vec_get(&file->line_starts, line_index);

  // 4. 填充结果 (全部 1-based)
  out_loc->filename = file->filename;
  out_loc->line = line_index + 1;
  out_loc->column = (local_offset - line_start_offset) + 1;

  return true;
}