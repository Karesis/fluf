#include <std/io/file.h>
#include <core/mem/allocer.h>
#include <core/mem/layout.h>
#include <core/msg/asrt.h>
#include <stdio.h> // C 标准 I/O: fopen, fseek, ftell, fread, fwrite, fclose

bool read_file_to_slice(allocer_t *alc, const char *path, str_slice_t *out_slice)
{
  asrt_msg(alc != NULL, "Allocator cannot be NULL");
  asrt_msg(path != NULL, "Path cannot be NULL");
  asrt_msg(out_slice != NULL, "Output slice cannot be NULL");

  FILE *file = fopen(path, "rb"); // "rb" = Read Binary
  if (file == NULL)
  {
    // 无法打开文件 (不存在, 权限问题等)
    return false;
  }

  // 1. 获取文件大小 (fseek/ftell)
  fseek(file, 0, SEEK_END);
  long file_size_long = ftell(file);
  if (file_size_long < 0)
  {
    // ftell 失败
    fclose(file);
    return false;
  }
  size_t file_size = (size_t)file_size_long;
  fseek(file, 0, SEEK_SET); // 倒带回文件开头

  // 2. 使用分配器分配内存
  // (大小 = file_size + 1 用于 '\0')
  layout_t layout = layout_of_array(char, file_size + 1);
  char *buffer = allocer_alloc(alc, layout);

  if (buffer == NULL)
  {
    // OOM (内存不足)
    fclose(file);
    return false;
  }

  // 3. 将文件读入内存
  size_t bytes_read = fread(buffer, 1, file_size, file);
  fclose(file); // 读完后立即关闭文件

  if (bytes_read != file_size)
  {
    // 读文件时出错 (例如，文件在读取时被截断)
    // 我们不需要释放 buffer，因为它是从 Arena 分配的
    // (这是 `fluf` 哲学的一部分：Arena 会处理清理)
    return false;
  }

  // 4. 设置 '\0' 终止符并返回
  buffer[file_size] = '\0';
  out_slice->ptr = buffer;
  out_slice->len = file_size;

  return true;
}

bool write_file_bytes(const char *path, const void *data, size_t len)
{
  asrt_msg(path != NULL, "Path cannot be NULL");
  asrt_msg(data != NULL, "Data cannot be NULL (unless len is 0)");

  FILE *file = fopen(path, "wb"); // "wb" = Write Binary (覆盖)
  if (file == NULL)
  {
    // 无法创建/打开文件 (权限问题)
    return false;
  }

  // 1. 写入数据
  if (len > 0)
  {
    size_t bytes_written = fwrite(data, 1, len, file);
    if (bytes_written != len)
    {
      // 写入失败 (磁盘空间已满?)
      fclose(file);
      return false;
    }
  }

  // 2. 成功
  fclose(file);
  return true;
}