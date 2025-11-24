# std/fs/path.h

## `static inline bool path_is_sep(char c) {`


Check if a character is a path separator.
Handles both '/' and '\\' on Windows for robustness.


---

## `str_t path_ext(str_t path);`


Get the file extension (without the dot).

Examples:
- "file.txt" -> "txt"
- "archive.tar.gz" -> "gz"
- "Makefile" -> "" (empty)
- ".gitignore" -> "gitignore" (Here we return "gitignore")
- "dir.with.dot/file" -> ""


---

## `str_t path_file_name(str_t path);`


Get the file name (basename).

Examples:
- "/usr/bin/gcc" -> "gcc"
- "src/main.c" -> "main.c"


---

## `str_t path_dir_name(str_t path);`


Get the directory part (dirname).

Examples:
- "/usr/bin/gcc" -> "/usr/bin"
- "file.txt" -> "" (or "." depending on philosophy. We return "" for pure relative)


---

## `[[nodiscard]] bool path_push(string_t *buf, str_t component);`


Push a component to a path buffer.
Automatically handles separator insertion/deduplication.


- **`buf`**: The path string builder (modified).
- **`component`**: The segment to append.
- **Returns**: true on success.

Logic:
"dir" + "file" -> "dir/file"
"dir/" + "file" -> "dir/file" (No double slash)
"" + "file" -> "file"


---

## `[[nodiscard]] bool path_set_ext(string_t *buf, str_t new_ext);`


Replace the extension of the path in the buffer.
* @param buf The path string builder.

- **`new_ext`**: The new extension (without dot).
- **Returns**: true on success.

Logic:
"image.png" + "jpg" -> "image.jpg"
"file" + "txt" -> "file.txt"


---

