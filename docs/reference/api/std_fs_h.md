# std/fs.h

## `bool file_exists(const char *path);`


Check if a file exists at the given path.

- **`path`**: C-string path.


---

## `bool file_remove(const char *path);`


Remove (delete) a file.

- **Returns**: true if successful, false if error (e.g., not found, permission).


---

## `[[nodiscard]] bool file_read_to_string(const char *path, string_t *out);`


Read the ENTIRE contents of a file into a string builder.


- **`path`**: Path to the file.
- **`out`**: Pointer to an initialized string_t.
The file content will be APPENDED to this string.
(Use string_clear(out) beforehand if you want to overwrite).


- **Returns**: true on success, false on failure.


> **Note:** 
1. Opens file in BINARY mode ("rb").
2. Intelligently pre-allocates memory to minimize reallocations.
3. Handles the null-terminator invariant of string_t automatically.


---

## `[[nodiscard]] bool file_write(const char *path, str_t content);`


Write a string slice to a file (Overwrite).


- **`path`**: Path to the file. Creates it if missing, truncates if exists.
- **`content`**: The data to write.
- **Returns**: true on success.


---

## `[[nodiscard]] bool file_append(const char *path, str_t content);`


Append a string slice to the end of a file.


- **`path`**: Path to the file. Creates it if missing.
- **`content`**: The data to append.
- **Returns**: true on success.


---

