# std/fs/srcmanager.h

## `usize srcmanager_add(srcmanager_t *mgr, str_t filename, str_t content);`


Add a file content to the manager.


- **`filename`**: Name of the file (copied internally).
- **`content`**: Content of the file (copied internally).
- **Returns**: File ID (index in the files vector), or (usize)-1 on failure.
* @note This calculates line endings immediately to build `line_starts`.


---

## `const srcfile_t *srcmanager_get_file(const srcmanager_t *mgr, usize id);`


Get a file by ID.


---

## `bool srcmanager_lookup(const srcmanager_t *mgr, usize offset, srcloc_t *out);`


Resolve a global offset to a SourceLocation (File, Line, Col).
* logic:
1. Binary search `mgr->files` to find which file contains the offset.
2. Calculate `local_offset = offset - file->base_offset`.
3. Binary search `file->line_starts` to find the line number.
4. Calculate column from the line start.
* @param out [out] Result struct.

- **Returns**: true if found, false if offset is out of bounds.


---

## `str_t srcmanager_get_line_content(const srcmanager_t *mgr, usize offset);`


Get the line content for a given location.
Useful for printing error messages with context code snippet.
* e.g., if offset points to "int x = 0;", this returns the whole line slice.


---

