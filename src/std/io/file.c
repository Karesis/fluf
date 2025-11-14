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

#include <core/mem/allocer.h>
#include <core/mem/layout.h>
#include <core/msg/asrt.h>
#include <std/io/file.h>
#include <stdio.h>

bool read_file_to_slice(allocer_t *alc, const char *path,
                        str_slice_t *out_slice) {
  asrt_msg(alc != NULL, "Allocator cannot be NULL");
  asrt_msg(path != NULL, "Path cannot be NULL");
  asrt_msg(out_slice != NULL, "Output slice cannot be NULL");

  FILE *file = fopen(path, "rb");
  if (file == NULL) {

    return false;
  }

  fseek(file, 0, SEEK_END);
  long file_size_long = ftell(file);
  if (file_size_long < 0) {

    fclose(file);
    return false;
  }
  size_t file_size = (size_t)file_size_long;
  fseek(file, 0, SEEK_SET);

  layout_t layout = layout_of_array(char, file_size + 1);
  char *buffer = allocer_alloc(alc, layout);

  if (buffer == NULL) {

    fclose(file);
    return false;
  }

  size_t bytes_read = fread(buffer, 1, file_size, file);
  fclose(file);

  if (bytes_read != file_size) {

    return false;
  }

  buffer[file_size] = '\0';
  out_slice->ptr = buffer;
  out_slice->len = file_size;

  return true;
}

bool write_file_bytes(const char *path, const void *data, size_t len) {
  asrt_msg(path != NULL, "Path cannot be NULL");
  asrt_msg(data != NULL, "Data cannot be NULL (unless len is 0)");

  FILE *file = fopen(path, "wb");
  if (file == NULL) {

    return false;
  }

  if (len > 0) {
    size_t bytes_written = fwrite(data, 1, len, file);
    if (bytes_written != len) {

      fclose(file);
      return false;
    }
  }

  fclose(file);
  return true;
}