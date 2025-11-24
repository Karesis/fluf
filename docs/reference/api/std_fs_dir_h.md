# std/fs/dir.h

## `typedef bool (*dir_walk_cb)(const char *path, dir_entry_type_t type, void *userdata);`


Callback for directory walking.


- **`path`**: The full path to the entry (e.g., "src/main.c").
NOTE: This pointer is ephemeral (valid only during callback).
If you need to keep it, copy it.

- **`type`**: The type of the entry.
- **`userdata`**: User context passed to dir_walk.
- **Returns**: true to continue walking, false to abort immediately.


---

## `bool dir_walk(allocer_t alc, const char *root, dir_walk_cb cb, void *userdata);`


Recursively walk a directory.


- **`alc`**: Allocator used for the internal path builder.
- **`root`**: Path to start walking from.
- **`cb`**: Callback function invoked for each entry.
- **`userdata`**: Passed to the callback.
- **Returns**: true if completed successfully, false if root is invalid or I/O error.


---

