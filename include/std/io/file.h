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

#include <core/mem/allocer.h>
#include <std/string/str_slice.h>
#include <stdbool.h>

/**
 * @brief (核心) 将整个文件读入由分配器管理的内存中。
 *
 * 这是编译器的主要 I/O 函数。它会分配一块大小为
 * (文件大小 + 1) 的内存，读入文件，并在末尾添加一个 '\0'。
 *
 * @param alc 用于分配内存的分配器 (通常是一个 Arena)。
 * @param path 文件的路径。
 * @param out_slice [out] 成功时，返回一个指向新分配内存的切片。
 * `out_slice.ptr` 是一个以 '\0' 结尾的 C 字符串。
 * `out_slice.len` 是文件的 *实际* 字节大小 (不含 \0)。
 * @return true (成功) 或 false (文件无法打开, 或 OOM)。
 */
bool read_file_to_slice(allocer_t *alc, const char *path,
                        strslice_t *out_slice);

/**
 * @brief 将一块内存写入文件。
 *
 * 这用于输出汇编、目标文件等。它会*覆盖*文件（如果已存在）。
 *
 * @param path 文件的路径。
 * @param data 要写入的字节。
 * @param len 要写入的字节数。
 * @return true (成功) 或 false (无法写入)。
 */
bool write_file_bytes(const char *path, const void *data, size_t len);