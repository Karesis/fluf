# std/io/file.h

## `bool read_file_to_slice(allocer_t *alc, const char *path, strslice_t *out_slice);`


(核心) 将整个文件读入由分配器管理的内存中。

这是编译器的主要 I/O 函数。它会分配一块大小为
(文件大小 + 1) 的内存，读入文件，并在末尾添加一个 '\0'。


- **`alc`**: 用于分配内存的分配器 (通常是一个 Arena)。
- **`path`**: 文件的路径。
- **`out_slice`**: [out] 成功时，返回一个指向新分配内存的切片。
`out_slice.ptr` 是一个以 '\0' 结尾的 C 字符串。
`out_slice.len` 是文件的 *实际* 字节大小 (不含 \0)。

- **Returns**: true (成功) 或 false (文件无法打开, 或 OOM)。


---

## `bool write_file_bytes(const char *path, const void *data, size_t len);`


将一块内存写入文件。

这用于输出汇编、目标文件等。它会*覆盖*文件（如果已存在）。


- **`path`**: 文件的路径。
- **`data`**: 要写入的字节。
- **`len`**: 要写入的字节数。
- **Returns**: true (成功) 或 false (无法写入)。


---

